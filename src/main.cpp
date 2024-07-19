#include "hittable.hpp"
#include "hittablelist.hpp"
#include "rtx.hpp"
#include "sphere.hpp"

using namespace jtx;

Color rayColor(const Rayd &r, const Hittable &world) {
    HitRecord rec;
    if (world.hit(r, Interval(0, INFINITY_D), rec)) {
        return 0.5 * (rec.normal + Color(1, 1, 1));
    }

    auto unit = normalize(r.dir);
    auto a = 0.5 * (unit.y + 1.0);
    return (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
}

// Image constants
constexpr double ASPECT_RATIO = 16.0 / 9.0;
constexpr int IMAGE_WIDTH = 400;
constexpr int IMAGE_HEIGHT = std::max(1, int(IMAGE_WIDTH / ASPECT_RATIO));

int main() {
    HittableList world;
    world.add(make_shared<Sphere>(Point3d(0, 0, -1), 0.5));
    world.add(make_shared<Sphere>(Point3d(0, -100.5, -1), 100));

    // Camera
    auto viewport_height = 2.0;
    auto viewport_width = viewport_height * (double(IMAGE_WIDTH) / IMAGE_HEIGHT);
    auto focal_length = 1.0;
    auto camera_center = Point3d(0, 0, 0);

    // Viewport vectors & deltas
    auto viewport_u = Vec3d(viewport_width, 0, 0);
    auto viewport_v = Vec3d(0, -viewport_height, 0);
    auto pixel_delta_u = viewport_u / IMAGE_WIDTH;
    auto pixel_delta_v = viewport_v / IMAGE_HEIGHT;

    // Pixel (0,0)
    auto viewport_upper_left = camera_center - Vec3d(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    std::cout
            << "P3\n"
            << IMAGE_WIDTH << ' ' << IMAGE_HEIGHT << "\n255\n";

    for (int j = 0; j < IMAGE_HEIGHT; ++j) {
        std::clog << "\rScanlines remaining: " << (IMAGE_HEIGHT - j) << ' ' << std::flush;
        for (int i = 0; i < IMAGE_WIDTH; ++i) {
            auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
            auto ray_dir = pixel_center - camera_center;
            Rayd r(camera_center, ray_dir);

            Color pixel_color = rayColor(r, world);
            write_color(std::cout, pixel_color);
        }
    }

    std::clog << "\rDone.                 \n";
}
