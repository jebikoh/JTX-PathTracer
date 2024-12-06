#include "color.hpp"
#include "rt.hpp"
#include "image.hpp"
#include "hittable.hpp"
#include "camera.hpp"
#include "material.hpp"

int main() {
    Camera camera{IMAGE_WIDTH, IMAGE_HEIGHT, 100, 10};

    // Materials
    auto lambertianGround = Lambertian(Color(0.8, 0.8, 0.0));
    auto lambertianCenter = Lambertian(Color(0.1, 0.2, 0.5));
    auto metalLeft        = Metal(Color(0.8, 0.8, 0.8), 0.3);
    auto metalRight       = Metal(Color(0.8, 0.6, 0.2), 1.0);

    auto materialGround = std::make_shared<Material>(&lambertianGround);
    auto materialCenter = std::make_shared<Material>(&lambertianCenter);
    auto materialLeft   = std::make_shared<Material>(&metalLeft);
    auto materialRight  = std::make_shared<Material>(&metalRight);

    // World
    auto centerSphere = Sphere(Vec3(0, 0, -1.2), 0.5, materialCenter);
    auto leftSphere   = Sphere(Vec3(-1, 0, -1), 0.5, materialLeft);
    auto rightSphere  = Sphere(Vec3(1, 0, -1), 0.5, materialRight);

    auto bigSphere = Sphere(Vec3(0, -100.5, -1), 100, materialGround);


    HittableList world;
    world.add(std::make_shared<Hittable>(&centerSphere));
    world.add(std::make_shared<Hittable>(&leftSphere));
    world.add(std::make_shared<Hittable>(&rightSphere));
    world.add(std::make_shared<Hittable>(&bigSphere));

    camera.render(world);
    camera.save("../output.png");
}
