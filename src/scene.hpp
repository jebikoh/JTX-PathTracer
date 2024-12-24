#pragma once
#include "camera.hpp"
#include "material.hpp"
#include "primitives.hpp"

// Very basic scene struct
// Will change if this starts running into performance issues
struct Scene {
    std::string name;

    std::vector<Material> materials;
    std::vector<Sphere> spheres;

    Camera::Properties cameraProperties;
};

Scene createDefaultScene(Scene &scene);
Scene createTestScene(Scene &scene);
Scene createCoverScene(Scene &scene);
