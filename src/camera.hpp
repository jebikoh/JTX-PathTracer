#pragma once

#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "rtx.hpp"
#include <jtxlib/util/rand.hpp>

class Camera {
public:
    double aspect_ratio   = 1.0;
    int image_width       = 100;
    int samples_per_pixel = 10;
    int max_depth         = 10;

    // Camera settings
    double vfov      = 90;
    Point3d lookFrom = {0, 0, 0};
    Point3d lookAt   = {0, 0, -1};
    Vec3d vup        = {0, 1, 0};

    double defocusAngle = 0;
    double focusDist    = 10;

    void render(const Hittable &world) {
        initialize();
        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << '\n'
                      << std::flush;
            for (int i = 0; i < image_width; ++i) {
                Color pixelColor(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    Rayd r = getRay(i, j);
                    pixelColor += rayColor(r, world, max_depth);
                }
                img.writePixel(pixel_samples_scale * pixelColor, j, i);
            }
        }
        std::clog << "\rDone.                        \n";
    }

    void save(const char *path) {
        img.save(path);
    }

private:
    int image_height;
    double pixel_samples_scale;
    Point3d center;
    Point3d pixel00_loc;
    Point3d pixel_delta_u;
    Point3d pixel_delta_v;
    RGBImage img;

    // Camera settings
    Vec3d u, v, w;
    Vec3d defocusDisk_u;
    Vec3d defocusDisk_v;

    void initialize() {
        image_height        = std::max(1, int(image_width / aspect_ratio));
        center              = lookFrom;
        pixel_samples_scale = 1.0 / samples_per_pixel;

        img.resize(image_width, image_height);

        auto theta           = jtx::radians(vfov);
        auto h               = tan(theta / 2);
        auto viewport_height = 2.0 * h * focusDist;
        auto viewport_width  = viewport_height * (double(image_width) / image_height);

        w = (lookFrom - lookAt).normalize();
        u = cross(vup, w).normalize();
        v = cross(w, u);

        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;
        pixel_delta_u   = viewport_u / image_width;
        pixel_delta_v   = viewport_v / image_height;

        auto viewport_upper_left = center - (focusDist * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc              = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocusRadius = focusDist * tan(radians(defocusAngle / 2));
        defocusDisk_u      = u * defocusRadius;
        defocusDisk_v      = v * defocusRadius;
    }

    [[nodiscard]] static Vec3d sampleSquare() {
        return {jtx::random<double>() - 0.5, jtx::random<double>(), 0};
    }

    // Constructs a camera ray from origin directed at randomly sampled point around (i, j)
    [[nodiscard]] Rayd getRay(int i, int j) const {
        auto offset      = sampleSquare();
        auto pixelSample = pixel00_loc + ((i + offset.x) * pixel_delta_u) + ((j + offset.y) * pixel_delta_v);
        auto rayOrigin   = (defocusAngle <= 0) ? center : defocusDiskSample();

        return {rayOrigin, pixelSample - rayOrigin};
    }

    [[nodiscard]] Point3d defocusDiskSample() const {
        auto p = randomInUnitDisk();
        return center + (p.x * defocusDisk_u) + (p.y * defocusDisk_v);
    }

    static Color rayColor(const Rayd &r, const Hittable &world, int depth) {
        if (depth <= 0) return {0, 0, 0};

        HitRecord rec;
        if (world.hit(r, Interval(0.001, INFINITY_D), rec)) {
            Rayd scattered;
            Color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                return attenuation * rayColor(scattered, world, depth - 1);
            }
            return {0, 0, 0};
        }

        auto unit = normalize(r.dir);
        auto a    = 0.8 * (unit.y + 1.0);
        return (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
    }
};
