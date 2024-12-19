#include "scene.hpp"

Scene createDefaultScene() {
    Scene scene{};
    scene.name = "Default Scene";

    // Camera
    scene.cameraProperties.center        = Vec3(-2, 2, 1);
    scene.cameraProperties.target        = Vec3(0, 0, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 10.0;
    scene.cameraProperties.focusDistance = 3.4;

    scene.lambertians.reserve(10);
    scene.metals.reserve(10);
    scene.dielectrics.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.lambertians.emplace_back(Color(0.8, 0.8, 0.0));
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, Material(&scene.lambertians.back()));

    scene.lambertians.emplace_back(Color(0.1, 0.2, 0.5));
    scene.spheres.emplace_back(Vec3(0, 0, -1.2), 0.5, Material(&scene.lambertians.back()));

    scene.metals.emplace_back(Color(0.8, 0.6, 0.2), 1.0);
    scene.spheres.emplace_back(Vec3(1, 0, -1), 0.5, Material(&scene.metals.back()));

    scene.dielectrics.emplace_back(1.5);
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.5, Material(&scene.dielectrics.back()));

    scene.dielectrics.emplace_back(1.00 / 1.50);
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.4, Material(&scene.dielectrics.back()));

    for (int i = 0; i < scene.spheres.size(); ++i) {
        scene.objects.add(Hittable(&scene.spheres[i]));
    }

    return scene;
}