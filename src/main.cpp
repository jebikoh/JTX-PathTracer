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
    Scene scene = createShaderBallScene();
    scene.buildBVH();

#ifndef DISABLE_UI
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
#else
    // Render the scene in orbit over 5 seconds @ 30 FPS
    constexpr int FPS = 30;
    constexpr int NUM_FRAMES = 5 * FPS;
    constexpr float TOTAL_ANGLE = 2 * PI;
    constexpr float DELTA_ANGLE = TOTAL_ANGLE / NUM_FRAMES;

    Camera camera{
        1920,
        1080,
        scene.cameraProperties,
        32,
        32,
        MAX_DEPTH};

    Vec3 offset = camera.properties_.center - camera.properties_.target;

    for (int frame = 142; frame < NUM_FRAMES; ++frame) {
        const float angle = frame * DELTA_ANGLE;

        const float cosTheta = jtx::cos(angle);
        const float sinTheta = jtx::sin(angle);

        const float x = offset.x * cosTheta - offset.z * sinTheta;
        const float z = offset.x * sinTheta + offset.z * cosTheta;
        Vec3 rotation{x, offset.y, z};

        camera.properties_.center = camera.properties_.target + rotation;

        std::cout << "Rendering frame: " << frame << std::endl;
        camera.render(scene);
        std::string path = "frame_" + std::to_string(frame) + ".png";
        camera.save(path.c_str());
        std::cout << "Saved frame: " << path << std::endl;
    }

    std::cout << "Finished rendering" << std::endl;

#endif

    scene.destroy();
    return 0;
}
