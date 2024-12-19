#include "camera.hpp"
#include "color.hpp"
#include "display.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "rt.hpp"
#include "scene.hpp"

#include <thread>

// Camera Settings
constexpr int IMAGE_WIDTH      = 800;
constexpr int IMAGE_HEIGHT     = 450;
constexpr Float ASPECT_RATIO   = static_cast<Float>(IMAGE_WIDTH) / static_cast<Float>(IMAGE_HEIGHT);
constexpr int SAMPLES_PER_PX   = 10;
constexpr int MAX_DEPTH        = 10;
constexpr Float YFOV           = 20;
const auto CAM_POS             = Vec3(-2, 2, 1);
const auto CAM_TARGET          = Vec3(0, 0, -1);
const auto CAM_UP              = Vec3(0, 1, 0);
constexpr Float DEFOCUS_ANGLE  = 10.0;
constexpr Float FOCUS_DISTANCE = 3.4;

int main() {
    Scene scene{};
    // createDefaultScene(scene);
    createCoverScene(scene);

    // return 0;
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

    BVHNode world(scene.objects);
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
    destroyBVHTree(&world, false);

    return 0;
}
