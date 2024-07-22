#include "camera.hpp"
#include "hittablelist.hpp"
#include "material.hpp"
#include "rtx.hpp"
#include "sphere.hpp"

using namespace jtx;

int main() {
    HittableList world;

    auto GroundMaterial = make_shared<Lambertian>(Color{0.5, 0.5, 0.5});
    world.add(make_shared<Sphere>(Point3d{0, -1000, 0}, 1000, GroundMaterial));

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            auto chooseMat = random<double>();
            Point3d center(a + 0.9 * random<double>(), 0.2, b + 0.9 * random<double>());

            if ((center - Point3d(4, 0.2, 0)).len() > 0.9) {
                shared_ptr<Material> sphereMaterial;

                if (chooseMat < 0.8) {
                    auto albedo    = Color::random() * Color::random();
                    sphereMaterial = make_shared<Lambertian>(albedo);
                    world.add(make_shared<Sphere>(center, 0.2, sphereMaterial));
                } else if (chooseMat < 0.95) {
                    auto albedo    = Color::random(0.5, 1);
                    auto fuzz      = random<double>(0, 0.5);
                    sphereMaterial = make_shared<Metal>(albedo, fuzz);
                    world.add(make_shared<Sphere>(center, 0.2, sphereMaterial));
                } else {
                    sphereMaterial = make_shared<Dielectric>(1.5);
                    world.add(make_shared<Sphere>(center, 0.2, sphereMaterial));
                }
            }
        }
    }

    auto Material1 = make_shared<Dielectric>(1.5);
    world.add(make_shared<Sphere>(Point3d(0, 1, 0), 1.0, Material1));

    auto Material2 = make_shared<Lambertian>(Color(0.4, 0.2, 0.1));
    world.add(make_shared<Sphere>(Point3d(-4, 1, 0), 1.0, Material2));

    auto Material3 = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<Sphere>(Point3d(4, 1, 0), 1.0, Material3));

    Camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 1200;
    cam.samples_per_pixel = 10;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookFrom = Point3d(13, 2, 3);
    cam.lookAt   = Point3d(0, 0, 0);
    cam.vup      = Vec3d(0, 1, 0);

    cam.defocusAngle = 0.6;
    cam.focusDist    = 10.0;

    cam.render(world);
    cam.save("image.png");
}
