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

    scene.materials.reserve(10);
    scene.spheres.reserve(10);

    // Objects & Materials
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.8, 0.8, 0.0)});
    scene.spheres.emplace_back(Vec3(0, -100.5, -1), 100, scene.materials.back());

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.1, 0.2, 0.5)});
    scene.spheres.emplace_back(Vec3(0, 0, -1.2), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 1.0});
    scene.spheres.emplace_back(Vec3(1, 0, -1), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5});
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.5, scene.materials.back());

    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.00 / 1.50});
    scene.spheres.emplace_back(Vec3(-1, 0, -1), 0.4, scene.materials.back());

    for (int i = 0; i < scene.spheres.size(); ++i) {
        scene.objects.add(Hittable(&scene.spheres[i]));
    }

    return scene;
}

Scene createTestScene(Scene &scene) {
    scene.cameraProperties.center        = Vec3(0, 3, 10);
    scene.cameraProperties.target        = Vec3(0, 2, -1);
    scene.cameraProperties.up            = Vec3(0, 1, 0);
    scene.cameraProperties.yfov          = 20;
    scene.cameraProperties.defocusAngle  = 0;
    scene.cameraProperties.focusDistance = 3.4;

    scene.materials.reserve(20);
    scene.spheres.reserve(20);

    // Glass bubbles
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5f});
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.0f / 1.5f});

    // Ground
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.8, 0.8, 0.0)});
    scene.spheres.push_back({Vec3(0, -100.5, -1), 100, scene.materials.back()});

    // Row one
    scene.spheres.push_back({Vec3(-1, 1, -1), 0.5, scene.materials[0]});
    scene.spheres.push_back({Vec3(-1, 1, -1), 0.4, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.1, 0.2, 0.5)});
    scene.spheres.push_back({Vec3(0, 1, -1), 0.5, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 1.0});
    scene.spheres.emplace_back(Vec3(1, 1, -1), 0.5, scene.materials.back());

    // Row two
    scene.spheres.push_back({Vec3(1, 2, -1), 0.5, scene.materials[0]});
    scene.spheres.push_back({Vec3(1, 2, -1), 0.4, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(1, 0.3, 0.5)});
    scene.spheres.push_back({Vec3(-1, 2, -1), 0.5, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.8, 0.8), .fuzz = 0.5});
    scene.spheres.emplace_back(Vec3(0, 2, -1), 0.5, scene.materials.back());

    // Row 3
    scene.spheres.push_back({Vec3(0, 3, -1), 0.5, scene.materials[0]});
    scene.spheres.push_back({Vec3(0, 3, -1), 0.4, scene.materials[1]});

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.5, 0.3, 0.5)});
    scene.spheres.push_back({Vec3(1, 3, -1), 0.5, scene.materials.back()});

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.8, 0.6, 0.2), .fuzz = 0});
    scene.spheres.emplace_back(Vec3(-1, 3, -1), 0.5, scene.materials.back());

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
    scene.materials.reserve(500);

    // Glass
    scene.materials.push_back({.type = Material::DIELECTRIC, .refractionIndex = 1.5});

    // Ground
    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.5, 0.5, 0.5)});
    scene.spheres.emplace_back(Vec3(0, -1000, 0), 1000, scene.materials.back());


    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            const Vec3 center(a + 0.9 * randomFloat(), 0.2, b + 0.9 * randomFloat());

            const auto matIdx = randomFloat();
            if ((center - Vec3(4, 0.2, 0)).len() > 0.9) {
                if (matIdx < DIFFUSE_PROBABILITY) {
                    auto albedo = randomVec3() * randomVec3();
                    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = albedo});
                    scene.spheres.emplace_back(center, 0.2, scene.materials.back());
                } else if (matIdx < METAL_CUTOFF) {
                    auto albedo = randomVec3(0.5, 1);
                    auto fuzz   = randomFloat(0, 0.5);
                    scene.materials.push_back({.type = Material::METAL, .albedo = albedo, .fuzz = fuzz});
                    scene.spheres.emplace_back(center, 0.2, scene.materials.back());
                } else {
                    // Dielectric glass always at the front
                    scene.spheres.emplace_back(center, 0.2, scene.materials.front());
                }
            }
        }
    }

    scene.spheres.emplace_back(Vec3(0, 1, 0), 1.0, scene.materials.front());

    scene.materials.push_back({.type = Material::LAMBERTIAN, .albedo = Color(0.4, 0.2, 0.1)});
    scene.spheres.emplace_back(Vec3(-4, 1, 0), 1.0, scene.materials.back());

    scene.materials.push_back({.type = Material::METAL, .albedo = Color(0.7, 0.6, 0.5), .fuzz = 0.0});
    scene.spheres.emplace_back(Vec3(4, 1, 0), 1.0, scene.materials.back());

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
