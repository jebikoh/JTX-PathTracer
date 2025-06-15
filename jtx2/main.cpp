#include <bvh/bvh.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <interface/display.hpp>
#include <scene/scene_loader.hpp>

// #define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();
#ifdef JTX_ENABLE_UI
    jtx::Scene scene;
    CHECK_JTX(jtx::LoadScene("assets/cornell_box.obj", scene));

    jtx::Display display;
    display.Init();
    display.SetScene(&scene);
    display.Run();
    display.Destroy();
#else
    jtx::Scene scene;
    jtx::LoadScene("assets/cb.obj", scene);
    scene.skyColor = vec3(0.0f);

    jtx::CameraSettings cs;
    cs.position = {278, 273, -800};
    cs.target = {278, 273, 0};
    cs.up = {0, 1, 0};
    cs.yfov = 40.0f;
    cs.focalDistance = 1.0f;

    jtx::RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 32;
    rs.sppCol = 32;
    rs.tileSize = 32;
    rs.numThreads = 16;
    rs.samplesPerPass = 16;

    jtx::BackendCPU backend;
    backend.Init(800, 400, rs, cs);
    backend.SetScene(&scene);
    backend.StartProgressiveRender();
    backend.Destroy();
#endif
    return 0;
}
