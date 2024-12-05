#include "color.hpp"
#include "rt.hpp"
#include "image.hpp"
#include "hittable.hpp"
#include "camera.hpp"

int main() {
    Camera camera{IMAGE_WIDTH, IMAGE_HEIGHT, 100};

    // World
    auto sphere1 = Sphere(Vec3(0, 0, -1), 0.5);
    auto sphere2 = Sphere(Vec3(0, -100.5, -1), 100);

    HittableList world;
    world.add(std::make_shared<Hittable>(&sphere1));
    world.add(std::make_shared<Hittable>(&sphere2));

    camera.render(world);
    camera.save("../output.png");
}
