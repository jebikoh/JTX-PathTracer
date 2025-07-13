#include <bvh/bvh.hpp>
#include <editor/editor.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <scene/scene_exporter.hpp>
#include <scene/scene_loader.hpp>
#include <thread>

#define JTX_ENABLE_UI

using namespace jtx;

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();
#ifdef JTX_ENABLE_UI
    Editor editor;
    editor.Init();
    editor.Run();
    editor.Destroy();
#else
    Scene scene;
    CHECK_JTX(LoadScene("../assets/scenes/knobs/knob.jtx", scene));
    // scene.envMap.uniform = vec3(0.733, 0.949, 1);
    // scene.envMap.intensity = 0.5f;
    // scene.envMap.type  = EnvMap::kType::UNIFORM;

    RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 32;
    rs.sppCol = 32;
    rs.tileSize = 32;
    rs.numThreads = std::thread::hardware_concurrency() - 1;
    rs.seed = 419;
    rs.samplesPerPass = 1;

    BackendCPU backend;
    backend.Init(800, 800, rs);
    backend.LoadScene(&scene);
    backend.StartOfflineRender();
    CHECK_JTX(backend.SaveRenderOutput("knob_e.png"));
    backend.Destroy();
#endif

    return 0;
}
