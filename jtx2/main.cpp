#include <backends/cpu_pt/bvh.hpp>
#include <jtx.hpp>
#include <scene/scene_loader.hpp>
#include <ui/display.hpp>

int main(int argc, char *argv[]) {
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::Display display;
    display.init();
    display.setScene(&scene);
    display.run();
    display.destroy();

    return 0;
}
