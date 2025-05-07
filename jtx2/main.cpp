#include <ui/display.hpp>
#include <scene/scene_loader.hpp>
#include <backends/cpu/bvh.hpp>
#include <util/validation.hpp>
#include <backends/cpu/sampling.hpp>

// #include <embree4/rtcore.h>
#include "util/rand.hpp"

int main(int argc, char *argv[]) {
    // jtx::Scene scene;
    // jtx::loadScene("assets/f22.obj", scene);
    //
    // // Init embree
    // const RTCDevice rtDevice = rtcNewDevice(nullptr);
    // const RTCScene rtScene   = rtcNewScene(rtDevice);
    // const RTCGeometry rtGeom = rtcNewGeometry(rtDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    //
    // const auto vertexBuffer = static_cast<float *>(rtcSetNewGeometryBuffer(rtGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(vec3), scene.positions.size()));
    // memcpy(vertexBuffer, scene.positions.data(), sizeof(vec3) * scene.positions.size());
    //
    // const auto indexBuffer = static_cast<unsigned *>(rtcSetNewGeometryBuffer(rtGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(vec3u), scene.indices.size()));
    // memcpy(indexBuffer, scene.indices.data(), sizeof(vec3u) * scene.indices.size());
    //
    // rtcCommitGeometry(rtGeom);
    // rtcAttachGeometry(rtScene, rtGeom);
    // rtcReleaseGeometry(rtGeom);
    // rtcCommitScene(rtScene);
    //
    // // Init jtx
    // jtx::BVH2 bvh;
    // bvh.build(scene);
    //
    // constexpr int SAMPLE_GRID_WIDTH = 100;
    // constexpr float SAMPLE_STEP = 1.0f / SAMPLE_GRID_WIDTH;
    // constexpr float z = 10.0f;
    // const vec3 dir = {0.0f, 0.0f, -1.0f};
    //
    // std::vector<float> embreeResult(SAMPLE_GRID_WIDTH);
    // std::vector<float> jtxResult(SAMPLE_GRID_WIDTH);
    //
    // uint32_t hitMatchCounter = 0;
    // uint32_t missMatchCounter = 0;
    // uint32_t diffCounter = 0;
    //
    // for (int row = 0; row < SAMPLE_GRID_WIDTH; ++row) {
    //     for (int col = 0; col < SAMPLE_GRID_WIDTH; ++col) {
    //         float y = row * SAMPLE_STEP;
    //         float x = col * SAMPLE_STEP;
    //
    //         // Embree
    //         RTCRayHit rayHit;
    //         rayHit.ray.org_x = x;
    //         rayHit.ray.org_y = y;
    //         rayHit.ray.org_z = z;
    //         rayHit.ray.dir_x = dir.x;
    //         rayHit.ray.dir_y = dir.y;
    //         rayHit.ray.dir_z = dir.z;
    //         rayHit.ray.tnear = 0.0f;
    //         rayHit.ray.tfar = jtx::JTX_INFINITY_F;
    //         rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    //
    //         RTCIntersectArguments ctx;
    //         rtcInitIntersectArguments(&ctx);
    //         rtcIntersect1(rtScene, &rayHit, &ctx);
    //
    //         bool bEmbreeHit = rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID;
    //
    //         // JTX
    //         jtx::SurfaceIntersection isect;
    //         const auto r = ray({x, y, z}, dir, 0);
    //         bool bJtxHit = bvh.closestHit(r, 0, jtx::JTX_INFINITY_F, isect);
    //
    //         if (bEmbreeHit == bJtxHit) {
    //             if (bEmbreeHit && bJtxHit) {
    //                 embreeResult.push_back(rayHit.ray.tfar);
    //                 jtxResult.push_back(isect.t);
    //                 hitMatchCounter++;
    //             } else {
    //                 embreeResult.push_back(0);
    //                 jtxResult.push_back(0);
    //                 missMatchCounter++;
    //             }
    //         } else {
    //             float embreeHit = bEmbreeHit ? rayHit.ray.tfar : -1.0f;
    //             float jtxHit = bJtxHit ? isect.t : -1.0f;
    //             diffCounter++;
    //             LOG_ERROR(GENERAL, "Delta detected: embree({}) | jtx({})", embreeHit, jtxHit);
    //         }
    //     }
    // }
    //
    // float mse = jtx::calculateMSE(jtxResult, embreeResult);
    // LOG_INFO(GENERAL, "MSE: {}", mse);
    // if (mse >= jtx::JTX_EPSILON) {
    //     LOG_INFO(GENERAL, "Failed epsilon test");
    // } else {
    //     LOG_INFO(GENERAL, "Passed epsilon test");
    // }
    //
    // LOG_INFO(GENERAL, "Number of matching hits: {}", hitMatchCounter);
    // LOG_INFO(GENERAL, "Number of matching misses: {}", missMatchCounter);
    // LOG_INFO(GENERAL, "Number of diffs: {}", diffCounter);
    //
    // rtcReleaseScene(rtScene);
    // rtcReleaseDevice(rtDevice);
    // bvh.destroy();

    // jtx::Display display;
    // display.init();
    // display.setScene(&scene);
    // display.run();
    // display.destroy();

    return 0;
}
