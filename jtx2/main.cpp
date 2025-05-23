#include <backends/cpu_pt/bvh.hpp>
#include <jtx.hpp>
#include <scene/scene_loader.hpp>
#include <ui/display.hpp>
#include <backends/cpu_pt/backend_cpu.hpp>
#include <backends/backends.hpp>

// #define JTX_ENABLE_UI

int main(int argc, char *argv[]) {
#ifdef JTX_ENABLE_UI
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::Display display;
    display.init();
    display.setScene(&scene);
    display.run();
    display.destroy();
#else
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::CameraSettings cs;
    jtx::RenderSettings rs;

    jtx::BackendCPU backend;
    backend.init(1920, 1080, rs, cs);
    backend.setScene(&scene);
    backend.startRendering();
    backend.destroy();
#endif
    return 0;
}
