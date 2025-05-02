#include <ui/display.hpp>
#include <scene/scene_loader.hpp>

int main(int argc, char *argv[]) {
    jtx::Scene scene;
    jtx::loadScene("assets/duck/Duck.obj", scene);

    jtx::Display display;
    display.init();
    display.setScene(&scene);
    display.run();
    display.destroy();

    return 0;
}
