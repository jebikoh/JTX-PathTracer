#include <SDL.h>

#include "bvh.hpp"
#include "camera.hpp"
#include "display.hpp"
#include "rt.hpp"
#include "scene.hpp"

#include <thread>

// Camera Settings
constexpr int IMAGE_WIDTH      = 800;
constexpr int IMAGE_HEIGHT     = 450;
constexpr int SAMPLES_PER_PX   = 200;
constexpr int MAX_DEPTH        = 10;

int main(int argc, char* argv[]) {
    Scene scene{};
//    createTestScene(scene);
//    createDefaultScene(scene);
    createCoverScene(scene);

    Camera camera{
        IMAGE_WIDTH,
        IMAGE_HEIGHT,
        scene.cameraProperties,
        SAMPLES_PER_PX,
        MAX_DEPTH
    };
    Display display(IMAGE_WIDTH + SIDEBAR_WIDTH, IMAGE_HEIGHT, &camera);
    if (!display.init()) {
        return -1;
    }

#ifdef USE_BVH_AS_WORLD
    const BVHTree world(scene, 1);
#else
    const PrimitiveList world(scene);
#endif

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

    display.destroy();

    return 0;
}
