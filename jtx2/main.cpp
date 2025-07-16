#include <bvh/bvh.hpp>
#include <editor/editor.hpp>
#include <engine/cpu/backend_cpu.hpp>
#include <scene/scene_loader.hpp>
#include <thread>

using namespace jtx;

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();

    Scene scene;
    CHECK_JTX(LoadScene("../assets/scenes/knobs/knob_hdri.jtx", scene));

    RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 8;
    rs.sppCol = 8;
    rs.tileSize = 32;
    rs.numThreads = std::thread::hardware_concurrency() - 1;
    rs.seed = 419;

    BackendCPU backend;
    backend.Init(1920, 1080, rs);
    backend.LoadScene(&scene);
    backend.StartOfflineRender();
    CHECK_JTX(backend.SaveRenderOutput("knob_d.png"));
    backend.Destroy();
    scene.Destroy();

    return 0;
}
