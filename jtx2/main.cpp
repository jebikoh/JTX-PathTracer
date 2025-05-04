#include <ui/display.hpp>
#include <scene/scene_loader.hpp>
#include <backends/cpu/bvh.hpp>

int main(int argc, char *argv[]) {

    jtx::Scene scene;
    jtx::loadScene("assets/duck/Duck.obj", scene);

    jtx::BVH2 bvh;
    bvh.build(scene);
    bvh.destroy();

    return 0; 

    // jtx::Display display;
    // display.init();
    // display.setScene(&scene);
    // display.run();
    // display.destroy();
    //
    // return 0;
}
