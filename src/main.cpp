#include <SDL.h>

#include "bvh.hpp"
#include "camera.hpp"
#include "display.hpp"
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

    // auto scene = createDefaultScene();
    // scene.cameraProperties.background = {0.2, 0.2, 0.2};

    auto t           = Mat4::identity();
    std::string path = "../src/assets/mitsuba.obj";
    auto scene       = createObjScene(path, t);

    scene.cameraProperties.center     = Vec3(0, 6, 12);
    scene.cameraProperties.target = Vec3(0, 0.6, 0);
    scene.cameraProperties.background = Color(0.7, 0.8, 1.0);

    const Vec3 GOLD_IOR = {0.15557, 0.42415, 1.3831};
    const Vec3 GOLD_K   = {-3.6024, -2.4721, -1.9155};

    scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.05, .alphaY = 0.05});
    scene.meshes[1].material = &scene.materials.back();

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.8, 0.8, 0.8)});
    scene.meshes[0].material = &scene.materials.back();

    scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.9, 0.9, 0.9)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, &scene.materials.back());

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
