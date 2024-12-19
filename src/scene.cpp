#include "scene.hpp"

Scene createDefaultScene(Scene &scene) {
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

Scene createCoverScene(Scene &scene) {
    constexpr float DIFFUSE_PROBABILITY = 0.8;
    constexpr float METAL_PROBABILITY   = 0.15;

    constexpr float METAL_CUTOFF = DIFFUSE_PROBABILITY + METAL_PROBABILITY;

    // There are around 500 spheres in this scene
    scene.spheres.reserve(500);

    scene.lambertians.reserve(500);
    scene.metals.reserve(200);
    scene.dielectrics.reserve(1);

    // Ground
    scene.lambertians.emplace_back(Color(0.5, 0.5, 0.5));
    scene.spheres.emplace_back(Vec3(0, -1000, 0), 1000, Material(&scene.lambertians.back()));

    // Glass
    scene.dielectrics.emplace_back(1.5);

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            const Vec3 center(a + 0.9 * randomFloat(), 0.2, b + 0.9 * randomFloat());

            const auto matIdx = randomFloat();
            if ((center - Vec3(4, 0.2, 0)).len() > 0.9) {
                if (matIdx < DIFFUSE_PROBABILITY) {
                    auto albedo = randomVec3() * randomVec3();
                    scene.lambertians.emplace_back(albedo);
                    scene.spheres.emplace_back(center, 0.2, Material(&scene.lambertians.back()));
                } else if (matIdx < METAL_CUTOFF) {
                    auto albedo = randomVec3(0.5, 1);
                    auto fuzz   = randomFloat(0, 0.5);
                    scene.metals.emplace_back(albedo, fuzz);
                    scene.spheres.emplace_back(center, 0.2, Material(&scene.metals.back()));
                } else {
                    scene.spheres.emplace_back(center, 0.2, Material(&scene.dielectrics.back()));
                }
            }
        }
    }

    scene.spheres.emplace_back(Vec3(0, 1, 0), 1.0, Material(&scene.dielectrics.back()));

    scene.lambertians.emplace_back(Vec3(0.4, 0.2, 0.1));
    scene.spheres.emplace_back(Vec3(-4, 1, 0), 1.0, Material(&scene.lambertians.back()));

    scene.metals.emplace_back(Color(0.7, 0.6, 0.5), 0.0);
    scene.spheres.emplace_back(Vec3(4, 1, 0), 1.0, Material(&scene.metals.back()));

    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.center        = {13, 2, 3};
    scene.cameraProperties.target        = {0, 0, 0};
    scene.cameraProperties.up            = {0, 1, 0};
    scene.cameraProperties.defocusAngle  = 0.6;
    scene.cameraProperties.focusDistance = 10.0;

    for (int i = 0; i < scene.spheres.size(); ++i) {
        scene.objects.add(Hittable(&scene.spheres[i]));
    }

    return scene;
}
