#pragma once
#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"

// Very basic scene struct
// Will change if this starts running into performance issues
struct Scene {
    std::string name;

    std::vector<Material> materials;
    std::vector<Sphere> spheres;

    HittableList objects;
    Camera::Properties cameraProperties;
};

Scene createDefaultScene(Scene &scene);
Scene createTestScene(Scene &scene);
Scene createCoverScene(Scene &scene);
