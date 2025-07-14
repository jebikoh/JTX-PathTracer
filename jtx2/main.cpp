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
    Image32f img;

    Scene scene;
    CHECK_JTX(LoadScene("../assets/scenes/knobs/knob_ne.jtx", scene));
    CHECK_JTX(Image32f::Load("../assets/hdri/envmap.exr", scene.envmap.HDRI));
    scene.envmap.type  = EnvMap::kType::HDRI;

    scene.cameraSettings.position.z = 24.0f;

    RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 16;
    rs.sppCol = 16;
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
