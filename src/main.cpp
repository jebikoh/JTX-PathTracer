#include <SDL.h>

#include "bvh.hpp"
#include "camera.hpp"
#include "display.hpp"
#include "rt.hpp"
#include "scene.hpp"

#include <thread>

// Camera Settings
constexpr int IMAGE_WIDTH    = 800;
constexpr int IMAGE_HEIGHT   = 400;
constexpr int MAX_DEPTH      = 50;

int main(int argc, char *argv[]) {
    Scene scene = createShaderBallSceneWithLight();
    scene.buildBVH();

    Camera camera{
            IMAGE_WIDTH,
            IMAGE_HEIGHT,
            scene.cameraProperties,
            8,
            8,
            MAX_DEPTH};

    Display display(IMAGE_WIDTH + SIDEBAR_WIDTH, IMAGE_HEIGHT, &camera);
    if (!display.init()) {
        return -1;
    }

    display.setScene(&scene);

    bool isRunning = true;
    while (isRunning) {
        display.processEvents(isRunning);
        display.render();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    while (display.isRendering()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    display.destroy();
    scene.destroy();
    return 0;
}
