#pragma once

#include "hittable.hpp"

constexpr int IMAGE_WIDTH    = 800;
constexpr int IMAGE_HEIGHT   = 450;
constexpr Float ASPECT_RATIO = static_cast<Float>(IMAGE_WIDTH) / static_cast<Float>(IMAGE_HEIGHT);

class Camera {
public:
    explicit Camera(
        const int width = IMAGE_WIDTH,
        const int height = IMAGE_HEIGHT,
        const Float aspectRatio = ASPECT_RATIO)
    {
        this->width       = width;
        this->height      = height;
        this->aspectRatio = aspectRatio;

        init();
    }

    void render(const HittableList &world) {
        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                auto pxCenter = vp00 + (i * du) + (j * dv);
                auto rayDirection = (pxCenter - center).normalize();
                Ray r(center, rayDirection);

                Color pixelColor = rayColor(r, world);
                img.writePixel(pixelColor, j, i);
            }
        }
    }

    void save(const char *path) const {
        img.save(path);
    }

private:
    int width, height;
    Float aspectRatio;

    Vec3 center;
    Vec3 vp00;
    Vec3 du;
    Vec3 dv;

    RGBImage img;

    void init() {
        center = Vec3(0, 0, 0);

        constexpr Float viewportHeight = 2.0;
        constexpr Float viewportWidth  = viewportHeight * ASPECT_RATIO;
        constexpr Float focalLength    = 1.0;

        img = RGBImage(width, height);

        // Viewport offsets
        const auto u = Vec3(viewportWidth, 0, 0);
        const auto v = Vec3(0, -viewportHeight, 0);
        du      = u / IMAGE_WIDTH;
        dv      = v / IMAGE_HEIGHT;

        // Viewport anchors
        const auto vpUpperLeft = center - Vec3(0, 0, focalLength) - u / 2 - v / 2;
        vp00        = vpUpperLeft + 0.5 * (du + dv);
    }

    static Color rayColor(const Ray &r, const HittableList &world) {
        // ReSharper disable once CppTooWideScopeInitStatement
        HitRecord record;
        if (world.hit(r, Interval(0, INF), record)) {
            return 0.5 * (record.normal + Color(1, 1, 1));
        }

        const auto a = 0.5 * (r.dir.y + 1.0);
        return jtx::lerp(Color(1, 1, 1), Color(0.5, 0.7, 1.0), a);
    }
};