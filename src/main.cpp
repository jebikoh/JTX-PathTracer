#include "camera.hpp"
#include "color.hpp"
#include "display.hpp"
#include "hittable.hpp"
#include "image.hpp"
#include "material.hpp"
#include "rt.hpp"

// Camera Settings
constexpr int IMAGE_WIDTH      = 400;
constexpr int IMAGE_HEIGHT     = 225;
constexpr Float ASPECT_RATIO   = static_cast<Float>(IMAGE_WIDTH) / static_cast<Float>(IMAGE_HEIGHT);
constexpr int SAMPLES_PER_PX   = 100;
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
    auto lambertianGround = Lambertian(Color(0.8, 0.8, 0.0));
    auto lambertianCenter = Lambertian(Color(0.1, 0.2, 0.5));
    auto metalLoFuzz      = Metal(Color(0.8, 0.8, 0.8), 0.3);
    auto metalHiFuzz      = Metal(Color(0.8, 0.6, 0.2), 1.0);
    auto glass            = Dielectric(1.5);
    auto glassBubble      = Dielectric(1.00 / 1.50);

    auto materialGround     = std::make_shared<Material>(&lambertianGround);
    auto materialCenter     = std::make_shared<Material>(&lambertianCenter);
    auto materialLeft       = std::make_shared<Material>(&glass);
    auto materialLeftBubble = std::make_shared<Material>(&glassBubble);
    auto materialRight      = std::make_shared<Material>(&metalHiFuzz);

    // World
    auto centerSphere = Sphere(Vec3(0, 0, -1.2), 0.5, materialCenter);
    auto leftSphere   = Sphere(Vec3(-1, 0, -1), 0.5, materialLeft);
    auto leftBubble   = Sphere(Vec3(-1, 0, -1), 0.4, materialLeftBubble);
    auto rightSphere  = Sphere(Vec3(1, 0, -1), 0.5, materialRight);

    auto bigSphere = Sphere(Vec3(0, -100.5, -1), 100, materialGround);


    HittableList world;
    world.add(std::make_shared<Hittable>(&centerSphere));
    world.add(std::make_shared<Hittable>(&leftSphere));
    world.add(std::make_shared<Hittable>(&leftBubble));
    world.add(std::make_shared<Hittable>(&rightSphere));
    world.add(std::make_shared<Hittable>(&bigSphere));

    camera.render(world);

    Display display(IMAGE_WIDTH, IMAGE_HEIGHT);
    if (!display.init()) {
        return -1;
    }

    bool isRunning = true;
    while (isRunning) {
        display.processEvents(isRunning);
        display.updateTexture(camera.image());
        display.render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}

