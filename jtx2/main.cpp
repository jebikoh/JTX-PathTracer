#include <bvh/bvh.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <interface/display.hpp>
#include <scene/scene_loader.hpp>

#define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();
#ifdef JTX_ENABLE_UI
    jtx::Scene scene;
    CHECK_JTX(jtx::LoadScene("assets/duck/duck.obj", scene));

    jtx::Display display;
    display.Init();
    display.SetScene(&scene);
    display.Run();
    display.Destroy();
#else
    jtx::Scene scene;
    jtx::LoadScene("assets/cornell_box/small/cb_small.obj", scene);

    scene.skyColor = vec3(0.0f);
    scene.cameraSettings.position = vec3{278, 273, -800} * 0.2f;
    scene.cameraSettings.target = vec3{278, 273, 0} * 0.2f;
    scene.cameraSettings.up = {0, 1, 0};
    scene.cameraSettings.yfov = 40.0f;
    scene.cameraSettings.focalDistance = 1.0f;

    jtx::RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 16;
    rs.sppCol = 16;
    rs.tileSize = 32;
    rs.numThreads = 16;
    rs.samplesPerPass = 16;

    jtx::BackendCPU backend;
    backend.Init(400, 400, rs);
    backend.LoadScene(&scene);
    backend.StartProgressiveRender();
    backend.Destroy();
#endif

    return 0;
}
