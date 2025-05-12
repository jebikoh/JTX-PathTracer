#include <gtest/gtest.h>

#include "../../util/rng.hpp"
#include "bvh.hpp"
#include "embree4/rtcore.h"

#include <functional>
#include <scene/scene_loader.hpp>

namespace jtx {

#ifdef JTX_BUILD_TESTS

const std::string JTX_BVH_TEST_MESH_PATH  = "assets/f22.obj";
constexpr int JTX_BVH_TEST_NUM_SAMPLES    = 10000;
constexpr int JTX_BVH_TEST_RNG_SEED       = 1234567;
constexpr float JTX_BVH_TEST_EPSILON      = 1e-5;
constexpr float JTX_BVH_TEST_OFFSET_SCALE = 5.0f;

void buildEmbreeTree(const Scene &scene, RTCDevice &device, RTCScene &eScene) {
    device                    = rtcNewDevice(nullptr);
    eScene                    = rtcNewScene(device);
    const RTCGeometry rtcGeom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    const auto vertexBuffer = static_cast<float *>(rtcSetNewGeometryBuffer(rtcGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(vec3), scene.positions.size()));
    memcpy(vertexBuffer, scene.positions.data(), sizeof(vec3) * scene.positions.size());

    const auto indexBuffer = static_cast<unsigned *>(rtcSetNewGeometryBuffer(rtcGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(vec3u), scene.indices.size()));
    memcpy(indexBuffer, scene.indices.data(), sizeof(vec3u) * scene.indices.size());

    rtcCommitGeometry(rtcGeom);
    rtcAttachGeometry(eScene, rtcGeom);
    rtcReleaseGeometry(rtcGeom);
    rtcCommitScene(eScene);
}

void destroyEmbreeTree(const RTCScene &scene, const RTCDevice &device) {
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

void validateBVH(
        const RTCScene &rtcScene,
        const std::function<void(const ray &, float &)> &isectFn,
        const std::function<ray(RNG &)> &rayGen,
        const uint32_t seed  = JTX_BVH_TEST_RNG_SEED,
        const int numSamples = JTX_BVH_TEST_NUM_SAMPLES,
        const float epsilon  = JTX_BVH_TEST_EPSILON) {
    RNG sampler{seed};
    float mse        = 0.0f;
    uint32_t trueHit = 0;

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

            rtcIntersect1(rtcScene, &rayHit);

            if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                tEmbree = rayHit.ray.tfar;
            }
        }

        isectFn(r, tJtx);

        bool bEmbreeHit = tEmbree >= 0.0f;
        bool bJtxHit    = tJtx >= 0.0f;
        EXPECT_EQ(bEmbreeHit, bJtxHit);

        if (bEmbreeHit && bJtxHit) {
            const float error = tEmbree - tJtx;
            mse += error * error;
            trueHit++;
        }
    }

    mse /= static_cast<float>(trueHit);
    EXPECT_LT(mse, epsilon);
}

class BVH2Test : public testing::Test {
protected:
    BVH2Test() {
        ASSERT(jtx::loadScene(JTX_BVH_TEST_MESH_PATH, m_scene));
        m_bvh.build(m_scene);
        buildEmbreeTree(m_scene, m_rtcDevice, m_rtcScene);
    }

    ~BVH2Test() override {
        destroyEmbreeTree(m_rtcScene, m_rtcDevice);
        m_bvh.destroy();
    }

    Scene m_scene;

    BVH2 m_bvh;

    const std::function<void(const ray &, float &)> m_isectFn = [&](const ray &r, float &t) {
        SurfaceIntersection isect;
        if (m_bvh.closestHit(r, 0, JTX_INFINITY_F, isect)) {
            t = isect.t;
        }
    };


    RTCDevice m_rtcDevice;
    RTCScene m_rtcScene;
};

TEST_F(BVH2Test, BVH2RaysToOrigin) {
    const auto rayGen = [](RNG &rng) {
        return detail::generateRayToOriginFromUnitSphere(rng, JTX_BVH_TEST_OFFSET_SCALE);
    };
    validateBVH(m_rtcScene, m_isectFn, rayGen);
}

TEST_F(BVH2Test, BVH2RandomRays) {
    const auto rayGen = [](RNG &rng) {
        return detail::generateRayFromUnitSphere(rng, JTX_BVH_TEST_OFFSET_SCALE);
    };
    validateBVH(m_rtcScene, m_isectFn, rayGen);
}

#endif

}// namespace jtx
