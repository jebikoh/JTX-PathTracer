#include <SDL.h>

#include "bvh.hpp"
#include "display.hpp"
#include "camera.hpp"
#include "rt.hpp"
#include "scene.hpp"

#include <thread>

// Camera Settings
//constexpr int IMAGE_WIDTH    = 600;
//constexpr int IMAGE_HEIGHT   = 600;
constexpr int IMAGE_WIDTH    = 800;
constexpr int IMAGE_HEIGHT   = 400;
constexpr int SAMPLES_PER_PX = 200;
constexpr int MAX_DEPTH      = 50;

int main(int argc, char *argv[]) {
    // const auto scene = createF22Scene();
//    const auto scene = createCornellBox();

    const auto scene = createDefaultScene();

    Camera camera{
        IMAGE_WIDTH,
        IMAGE_HEIGHT,
        scene.cameraProperties,
        SAMPLES_PER_PX,
        MAX_DEPTH};
    camera.xPixelSamples_ = 8;
    camera.yPixelSamples_ = 8;

    Display display(IMAGE_WIDTH + SIDEBAR_WIDTH, IMAGE_HEIGHT, &camera);
    if (!display.init()) {
        return -1;
    }

    BVHTree world(scene, 1);

    display.setWorld(&world);

    bool isRunning = true;
    while (isRunning) {
        display.processEvents(isRunning);
        display.render();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    while (display.isRendering()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    world.destroy();
    display.destroy();
    scene.destroy();
    return 0;
}
