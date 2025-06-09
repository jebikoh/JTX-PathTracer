#include <backends/cpu_pt/bvh.hpp>
#include <jtx.hpp>
#include <scene/scene_loader.hpp>
#include <ui/display.hpp>
#include <backends/cpu_pt/backend_cpu.hpp>
#include <backends/backends.hpp>

#define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
#ifdef JTX_ENABLE_UI
    jtx::Scene scene;
    // jtx::LoadScene("assets/f22.obj", scene);

    // jtx::Display display;
    // display.Init();
    // display.SetScene(&scene);
    // display.Run();
    // display.Destroy();
#else
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::CameraSettings cs;
    cs.position = {0.0f, 0.0f, 10.0f};
    cs.target = {0.0f, 0.0f, 0.0f};
    cs.up = {0.0f, 1.0f, 0.0f};
    cs.yfov = 40.0f;
    cs.focalDistance = 1.0f;

    jtx::RenderSettings rs;
    rs.maxDepth = 32;
    rs.sppRow = 32;
    rs.sppCol = 32;
    rs.tileSize = 32;
    rs.numThreads = 16;
    rs.samplesPerPass = 1;

    jtx::BackendCPU backend;
    backend.init(800, 400, rs, cs);
    backend.setScene(&scene);
    backend.startRendering();
    backend.destroy();
#endif
    return 0;
}
