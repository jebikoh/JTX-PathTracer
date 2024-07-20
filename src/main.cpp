#include "camera.hpp"
#include "hittablelist.hpp"
#include "material.hpp"
#include "rtx.hpp"
#include "sphere.hpp"

using namespace jtx;

int main() {
    HittableList world;

    auto MatGround = make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
    auto MatCenter = make_shared<Lambertian>(Color(0.1, 0.2, 0.5));
    auto MatLeft   = make_shared<Metal>(Color(0.8, 0.8, 0.8));
    auto MatRight  = make_shared<Metal>(Color(0.8, 0.6, 0.2));

    world.add(make_shared<Sphere>(Point3d(0.0, -100.5, -1.0), 100.0, MatGround));
    //    world.add(make_shared<Sphere>(Point3d(0.0, 0.0, -1.2), 0.5, MatCenter));
    //    world.add(make_shared<Sphere>(Point3d(-1.0, 0.0, -1.0), 0.5, MatLeft));
    //    world.add(make_shared<Sphere>(Point3d(1.0, 0.0, -1.0), 0.5, MatRight));

    Camera cam;
    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.render(world);
    cam.save("image.png");
}
