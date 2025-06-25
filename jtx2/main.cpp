#include <bvh/bvh.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <interface/display.hpp>
#include <scene/scene_loader.hpp>
#include <thread>
#include <scene/scene_exporter.hpp>

#define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();
#ifdef JTX_ENABLE_UI
    jtx::Display display;
    display.Init();
    display.Run();
    display.Destroy();
#else
    jtx::Scene scene;
    CHECK_JTX(jtx::LoadScene("../assets/scenes/ajax/ajax.obj", scene));
    CHECK_JTX(jtx::ExportScene(scene, "../assets/scenes/ajax/ajax.jtx"));

    // jtx::Scene scene;
    // CHECK_JTX(jtx::LoadScene("ajax.jtx", scene));
    //
    // jtx::RenderSettings rs;
    // rs.maxDepth = 32;
    // rs.sppRow = 16;
    // rs.sppCol = 16;
    // rs.tileSize = 32;
    // rs.numThreads = std::thread::hardware_concurrency() - 1;
    // rs.seed = 419;
    // rs.samplesPerPass = 1;
    //
    // jtx::BackendCPU backend;
    // backend.Init(400, 400, rs);
    // backend.LoadScene(&scene);
    // backend.StartOfflineRender();
    // CHECK_JTX(backend.SaveRenderOutput("ajax_rough_dielectric.png"));
    // backend.Destroy();
#endif

    return 0;
}
