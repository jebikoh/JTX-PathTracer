#include "camera.hpp"
#include "color.hpp"
#include "display.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "rt.hpp"

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
    Camera camera{
            IMAGE_WIDTH,
            IMAGE_HEIGHT,
            YFOV,
            CAM_POS,
            CAM_TARGET,
            CAM_UP,
            DEFOCUS_ANGLE,
            FOCUS_DISTANCE,
            SAMPLES_PER_PX,
            MAX_DEPTH};

    // Materials
    auto lambertianGround    = Lambertian(Color(0.8, 0.8, 0.0));
    auto lambertianCenter    = Lambertian(Color(0.1, 0.2, 0.5));
    auto metalLoFuzz    = Metal(Color(0.8, 0.8, 0.8), 0.3);
    auto metalHiFuzz    = Metal(Color(0.8, 0.6, 0.2), 1.0);
    auto glass    = Dielectric(1.5);
    auto glassBubble    = Dielectric(1.00 / 1.50);


    // World
    auto centerSphere = Sphere(Vec3(0, 0, -1.2), 0.5, Material(&lambertianCenter));
    auto leftSphere   = Sphere(Vec3(-1, 0, -1), 0.5, Material(&glass));
    auto leftBubble   = Sphere(Vec3(-1, 0, -1), 0.4, Material(&glassBubble));
    auto rightSphere  = Sphere(Vec3(1, 0, -1), 0.5, Material(&metalHiFuzz));
    auto groundSphere    = Sphere(Vec3(0, -100.5, -1), 100, Material(&lambertianGround));

    HittableList objects;
    objects.add(Hittable(&centerSphere));
    objects.add(Hittable(&leftSphere));
    objects.add(Hittable(&leftBubble));
    objects.add(Hittable(&rightSphere));
    objects.add(Hittable(&groundSphere));
    BVHNode bvh(objects);
    HittableList world;
    world.add(Hittable(&bvh));

    Display display(IMAGE_WIDTH + SIDEBAR_WIDTH, IMAGE_HEIGHT, &camera);
    if (!display.init()) {
        return -1;
    }
    display.setWorld(&world);

    bool isRunning = true;
    while (isRunning) {
        display.processEvents(isRunning);
        display.render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    display.destroy();
    camera.save("../output.png");

    destroyBVHTree(&bvh, false);

    return 0;
}
