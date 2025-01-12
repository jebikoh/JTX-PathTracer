#include <SDL.h>

#include "bvh.hpp"
#include "display.hpp"
#include "camera.hpp"
#include "rt.hpp"
#include "scene.hpp"

#include <thread>

// Camera Settings
constexpr int IMAGE_WIDTH    = 800;
constexpr int IMAGE_HEIGHT   = 450;
constexpr int SAMPLES_PER_PX = 200;
constexpr int MAX_DEPTH      = 10;

int main(int argc, char *argv[]) {
    Scene scene;
    scene.loadMesh("../src/assets/f22.obj");

    scene.cameraProperties.center        = Vec3(0, 0, 8);
    scene.cameraProperties.target        = Vec3(0, 0, 0);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 1;

    std::cout << scene.meshes[0].numVertices << std::endl;
    std::cout << scene.meshes[0].numIndices << std::endl;

    // scene.meshes[0].material.type = Material::DIELECTRIC;
    // scene.meshes[0].material.refractionIndex = 1.5f;

    scene.meshes[0].material.type = Material::METAL;
    scene.meshes[0].material.albedo =  Color(0.8, 0.6, 0.2);
    scene.meshes[0].material.fuzz = 0.25;

    jtx::Mat4 m = jtx::rotateX(25);
    // // Transform all verts and norms
    for (int i = 0; i < scene.meshes[0].numVertices; ++i) {
        scene.meshes[0].vertices[i] = m.applyToPoint(scene.meshes[0].vertices[i]);
        scene.meshes[0].normals[i]  = m.applyToNormal(scene.meshes[0].normals[i]);
    }

    Camera camera{
            IMAGE_WIDTH,
            IMAGE_HEIGHT,
            scene.cameraProperties,
            SAMPLES_PER_PX,
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
