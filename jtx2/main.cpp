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
    CHECK_JTX(LoadScene("../assets/scenes/knobs/knob_hdri.jtx", scene));
    // CHECK_JTX(Image32f::Load("../assets/hdri/envmap.exr", scene.envmap.image));
    // scene.envmap.type             = EnvMap::kType::HDRI;
    // scene.envmap.horizontalOffset = Radians(90);
    scene.cameraSettings.position.z = 24.0f;
    scene.cameraSettings.focalLength = 0.01f;
    // ExportScene(scene, "../assets/scenes/knobs/knob_hdri.jtx");
    // scene.Destroy();

    RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 8;
    rs.sppCol = 8;
    rs.tileSize = 32;
    rs.numThreads = std::thread::hardware_concurrency() - 1;
    rs.seed = 419;
    rs.samplesPerPass = 1;

    BackendCPU backend;
    backend.Init(1920, 1080, rs);
    backend.LoadScene(&scene);
    backend.StartOfflineRender();
    CHECK_JTX(backend.SaveRenderOutput("knob_d.png"));
    backend.Destroy();
    scene.Destroy();
#endif

    return 0;
}
