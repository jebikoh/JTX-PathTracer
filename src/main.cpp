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

    auto scene = createDefaultScene();
    scene.cameraProperties.background = {0.2, 0.2, 0.2};

    // const Mat4 t = Mat4::identity();
    // std::string path = "../src/assets/f22.obj";
    // auto scene = createObjScene(path, t);
    //
    // const Vec3 GOLD_IOR = {0.15557,0.42415,1.3831};
    // const Vec3 GOLD_K   = {-3.6024,-2.4721,-1.9155};
    //
    // scene.cameraProperties.background = {0.2, 0.2, 0.2};
    //
    // // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.1, 0.2, 0.5)});
    // scene.materials.push_back({.type = Material::CONDUCTOR, .IOR = GOLD_IOR, .k = GOLD_K, .alphaX = 0.1, .alphaY = 0.1});
    // scene.meshes.back().material = &scene.materials.back();
    //
    // scene.materials.push_back({.type = Material::DIFFUSE, .albedo = Color(0.659, 0.659, 0.749)});
    // scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, &scene.materials.back());

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
