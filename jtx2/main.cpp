#include <backends/cpu/bvh.hpp>
#include <scene/scene_loader.hpp>
#include <ui/display.hpp>
#include <util/validation.hpp>

#include <backends/cpu/rng.hpp>
#include <backends/cpu/sampling.hpp>
#include <embree4/rtcore.h>

int main(int argc, char *argv[]) {
    constexpr int NUM_SAMPLES = 1000000;
    jtx::rng rng{23, 17, 19};

    // Load scene
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    // Init embree
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
    //
    // // Init custom BVH
    // jtx::BVH2 bvh{};
    // bvh.build(scene);
    //
    // // Setup stats
    // uint32_t bothMiss     = 0;
    // uint32_t bothHit      = 0;
    // uint32_t oneHitJtx    = 0;
    // uint32_t oneHitEmbree = 0;
    //
    // // For each iteration, we will...
    // // 1. Generate a point p on unit sphere
    // // 2. Scale p away from origin by set distance
    // // 3. Perform intersection on ray from p to the origin
    // const vec3 o{};
    // for (int i = 0; i < NUM_SAMPLES; ++i) {
    //     // Generate ray
    //     vec3 p = rng.unitSphere();
    //     p *= 5;
    //     vec3 d = (o - p).normalize();
    //
    //     float tEmbree = -1.0f;
    //     float tJtx = -1.0f;
    //
    //     // Test embree
    //     {
    //         RTCRayHit rayHit;
    //         rayHit.ray.org_x  = p.x; rayHit.ray.org_y = p.y; rayHit.ray.org_z = p.z;
    //         rayHit.ray.dir_x  = d.x; rayHit.ray.dir_y = d.y; rayHit.ray.dir_z = d.y;
    //         rayHit.ray.tnear  = 0.f;
    //         rayHit.ray.tfar   = jtx::JTX_INFINITY_F;
    //         rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    //
    //         RTCIntersectArguments ctx;
    //         rtcInitIntersectArguments(&ctx);
    //         rtcIntersect1(rtScene, &rayHit, &ctx);
    //
    //         if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
    //             tEmbree = rayHit.ray.tfar;
    //         }
    //     }
    //
    //     // Test JTX
    //     {
    //         jtx::SurfaceIntersection isect{};
    //         ray r{p, d, 0};
    //         if (bvh.closestHit(r, 0, jtx::JTX_INFINITY_F, isect)) {
    //             tJtx = isect.t;
    //         }
    //     }
    //
    //     if (tEmbree > 0.0f && tJtx > 0.0f) {
    //         bothHit++;
    //         continue;
    //     }
    //
    //     if (tEmbree < 0.0f && tJtx < 0.0f) {
    //         bothMiss++;
    //         continue;
    //     }
    //
    //     if (tEmbree < 0.0f && tJtx > 0.0f) {
    //         oneHitJtx++;
    //         continue;
    //     }
    //
    //     if (tEmbree > 0.0f && tJtx < 0.0f) {
    //         oneHitEmbree++;
    //         continue;
    //     }
    // }
    //
    // LOG_INFO(GENERAL, "Both hit: {}", bothHit);
    // LOG_INFO(GENERAL, "Both miss: {}", bothMiss);
    // LOG_INFO(GENERAL, "One hit (JTX): {}", oneHitJtx);
    // LOG_INFO(GENERAL, "One hit (embree): {}", oneHitEmbree);
    //
    // // Cleanup
    rtcReleaseScene(rtScene);
    rtcReleaseDevice(rtDevice);
    // bvh.destroy();

    return 0;
}
