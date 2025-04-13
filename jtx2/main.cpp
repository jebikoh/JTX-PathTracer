#include <ui/display.hpp>
#include <scene/scene_loader.hpp>

int main(int argc, char *argv[]) {
    jtx::Scene scene;
    jtx::loadScene("assets/f22.obj", scene);

    jtx::Display display;
    display.init();
    display.setScene(&scene);
    display.cleanup();

    return 0;
}
