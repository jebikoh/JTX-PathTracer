#pragma once

#include "bvh.hpp"
#include "embree4/rtcore.h"
#include "rng.hpp"

#include <functional>

namespace jtx {

constexpr int JTX_BVH_TEST_NUM_SAMPLES = 1000000;

struct BVHValidationResults {
    uint32_t trueHit   = 0;
    uint32_t trueMiss  = 0;
    uint32_t falseHit  = 0;
    uint32_t falseMiss = 0;
    float hitMSE       = 0.0f;
    float matchPct     = 0.0f;
    bool passed        = false;

    void logResults() {
        LOG_INFO(GENERAL, "Both hit: {}", trueHit);
        LOG_INFO(GENERAL, "Both miss: {}", trueMiss);
        LOG_INFO(GENERAL, "False misses: {}", falseMiss);
        LOG_INFO(GENERAL, "False hits: {}", falseHit);
        LOG_INFO(GENERAL, "Match percentage: {}", matchPct);
        LOG_INFO(GENERAL, "t MSE: {}", hitMSE);
        LOG_INFO(GENERAL, "Passed validation?: {}", passed);
    }
};

namespace detail {
    inline ray generateRayToOrigin(RNG &rng, const float offsetScale) {
        ray out{};
        out.origin = rng.unitSphere() * offsetScale;
        out.dir    = (JTX_VEC3_ORIGIN - out.origin).normalize();
        return out;
    }

    inline ray generateRandomRay(RNG &rng, const float offsetScale) {
        ray out{};
        out.origin    = rng.unitSphere() * offsetScale;
        const vec3 p1 = rng.unitSphere() * offsetScale;
        out.dir       = (p1 - out.origin).normalize();
        return out;
    }

    inline BVHValidationResults validateBVH(
            const Scene &scene,
            const std::function<void(const Scene &)> &loadFn,
            const std::function<void(const ray &, float &)> &isectFn,
            const std::function<void()> &destroyFn,
            const std::function<ray(RNG &)> &rayGen,
            const uint32_t seed,
            const int numSamples,
            const float epsilon) {
        BVHValidationResults results{};
        const vec3 o{};
        RNG sampler{seed};

        LOG_INFO(GENERAL, "Testing BVH using Embree");

        // Init Embree
        const RTCDevice rtDevice = rtcNewDevice(nullptr);
        const RTCScene rtScene   = rtcNewScene(rtDevice);
        const RTCGeometry rtGeom = rtcNewGeometry(rtDevice, RTC_GEOMETRY_TYPE_TRIANGLE);

        const auto vertexBuffer = static_cast<float *>(rtcSetNewGeometryBuffer(rtGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(vec3), scene.positions.size()));
        memcpy(vertexBuffer, scene.positions.data(), sizeof(vec3) * scene.positions.size());

        const auto indexBuffer = static_cast<unsigned *>(rtcSetNewGeometryBuffer(rtGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(vec3u), scene.indices.size()));
        memcpy(indexBuffer, scene.indices.data(), sizeof(vec3u) * scene.indices.size());

        rtcCommitGeometry(rtGeom);
        rtcAttachGeometry(rtScene, rtGeom);
        rtcReleaseGeometry(rtGeom);
        rtcCommitScene(rtScene);

        // Init test BVH
        loadFn(scene);

        for (int i = 0; i < numSamples; ++i) {
            // Generate point
            ray r = rayGen(sampler);

            float tEmbree = -1.0f;
            float tJtx    = -1.0f;

            // Test embree
            {
                RTCRayHit rayHit;
                rayHit.ray.org_x     = r.origin.x;
                rayHit.ray.org_y     = r.origin.y;
                rayHit.ray.org_z     = r.origin.z;
                rayHit.ray.dir_x     = r.dir.x;
                rayHit.ray.dir_y     = r.dir.y;
                rayHit.ray.dir_z     = r.dir.z;
                rayHit.ray.tnear     = 0.f;
                rayHit.ray.tfar      = jtx::JTX_INFINITY_F;
                rayHit.ray.mask      = -1;
                rayHit.ray.flags     = 0;
                rayHit.hit.geomID    = RTC_INVALID_GEOMETRY_ID;
                rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

                rtcIntersect1(rtScene, &rayHit);

                if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    tEmbree = rayHit.ray.tfar;
                }
            }

            isectFn(r, tJtx);

            if (tEmbree > 0.0f && tJtx > 0.0f) {
                results.trueHit++;
                const float error = tEmbree - tJtx;
                results.hitMSE += error * error;
                continue;
            }

            if (tEmbree < 0.0f && tJtx < 0.0f) {
                results.trueMiss++;
                continue;
            }

            if (tEmbree < 0.0f && tJtx > 0.0f) {
                results.falseHit++;
                continue;
            }

            if (tEmbree > 0.0f && tJtx < 0.0f) {
                results.falseMiss++;
            }
        }

        rtcReleaseScene(rtScene);
        rtcReleaseDevice(rtDevice);
        destroyFn();


        results.hitMSE   = results.trueHit == 0 ? 0.0f : results.hitMSE / results.trueHit;
        results.matchPct = static_cast<float>(results.trueHit + results.trueMiss) / static_cast<float>(numSamples);
        results.passed   = results.falseHit == 0 && results.falseMiss == 0 && results.hitMSE < epsilon;

        return results;
    }
}// namespace detail

enum BVHValidationMethod {
    JTX_BVH_VALIDATION_METHOD_RANDOM_RAYS           = 0,
    JTX_BVH_VALIDATION_METHOD_RANDOM_RAYS_TO_ORIGIN = 1,
};

inline BVHValidationResults validateBVH2(const Scene &scene,
                                         BVHValidationMethod method = JTX_BVH_VALIDATION_METHOD_RANDOM_RAYS,
                                         const float offsetScale    = 2.0f,
                                         const uint32_t seed        = 0,
                                         const int numSamples       = JTX_BVH_TEST_NUM_SAMPLES,
                                         const float epsilon        = JTX_EPSILON) {
    jtx::BVH2 bvh;
    const auto loadFn = [&](const Scene &s) {
        bvh.build(s);
    };

    const auto isectFn = [&](const ray &r, float &t) {
        SurfaceIntersection isect;
        if (bvh.closestHit(r, 0, JTX_INFINITY_F, isect)) {
            t = isect.t;
        }
    };

    const auto destroyFn = [&]() {
        bvh.destroy();
    };

    std::function<ray(RNG &rng)> rayGen;
    switch (method) {
        case JTX_BVH_VALIDATION_METHOD_RANDOM_RAYS:
            rayGen = [&](RNG &rng) {
                return detail::generateRandomRay(rng, offsetScale);
            };
            break;
        case JTX_BVH_VALIDATION_METHOD_RANDOM_RAYS_TO_ORIGIN:
        default:
            rayGen = [&](RNG &rng) {
                return detail::generateRayToOrigin(rng, offsetScale);
            };
            break;
    }

    return detail::validateBVH(scene, loadFn, isectFn, destroyFn, rayGen, seed, numSamples, epsilon);
}

}// namespace jtx