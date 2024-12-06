#pragma once

#include "hittable.hpp"
#include "interval.hpp"
#include "material.hpp"

constexpr int IMAGE_WIDTH    = 400;
constexpr int IMAGE_HEIGHT   = 224;
constexpr Float ASPECT_RATIO = static_cast<Float>(IMAGE_WIDTH) / static_cast<Float>(IMAGE_HEIGHT);
constexpr int SAMPLES_PER_PX = 10;
constexpr int MAX_DEPTH      = 10;

class Camera {
public:
    explicit Camera(
            const int width         = IMAGE_WIDTH,
            const int height        = IMAGE_HEIGHT,
            const int samplesPerPx  = SAMPLES_PER_PX,
            const int maxDepth      = MAX_DEPTH) {
        // No initializer list cuz its long and ugly to read here
        this->width        = width;
        this->height       = height;
        this->aspectRatio  = static_cast<Float>(width) / static_cast<Float>(height);
        this->samplesPerPx = samplesPerPx;
        this->maxDepth     = maxDepth;

        init();
    }

    void render(const HittableList &world) {
        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                auto pxColor = Color(0, 0, 0);
                for (int s = 0; s < samplesPerPx; ++s) {
                    Ray r = getRay(i, j);
                    pxColor += rayColor(r, world, maxDepth);
                }
                img.writePixel(pxColor * pxSampleScale, j, i);
            }
        }
    }

    void save(const char *path) const {
        img.save(path);
    }

private:
    int width, height;
    Float aspectRatio;
    RGBImage img;

    int samplesPerPx;
    Float pxSampleScale;
    int maxDepth;

    Vec3 center;
    Vec3 vp00;
    Vec3 du;
    Vec3 dv;

    void init() {
        center = Vec3(0, 0, 0);

        constexpr Float viewportHeight = 2.0;
        constexpr Float viewportWidth  = viewportHeight * ASPECT_RATIO;
        constexpr Float focalLength    = 1.0;

        img = RGBImage(width, height);

        pxSampleScale = static_cast<Float>(1.0) / static_cast<Float>(samplesPerPx);

        // Viewport offsets
        const auto u = Vec3(viewportWidth, 0, 0);
        const auto v = Vec3(0, -viewportHeight, 0);
        du           = u / IMAGE_WIDTH;
        dv           = v / IMAGE_HEIGHT;

        // Viewport anchors
        const auto vpUpperLeft = center - Vec3(0, 0, focalLength) - u / 2 - v / 2;
        vp00                   = vpUpperLeft + 0.5 * (du + dv);
    }

    Ray getRay(int i, int j) {
        const auto offset = sampleSquare();
        const auto sample = vp00 + ((i + offset.x) * du) + ((j + offset.y) * dv);
        return {center, sample - center};
    }

    static Vec3 sampleSquare() {
        return {randomFloat() - static_cast<Float>(0.5), randomFloat() - static_cast<Float>(0.5), 0};
    }

    static Color rayColor(const Ray &r, const HittableList &world, int depth) {
        Ray currRay = r;
        Color currAttenuation = {1.0, 1.0, 1.0};
        for (int i = 0; i < depth; ++i) {
            HitRecord record;
            if (world.hit(currRay, Interval(0.001, INF), record)) {
                Ray scattered;
                Color attenuation;
                if (record.material->scatter(currRay, record, attenuation, scattered)) {
                    currAttenuation *= attenuation;
                    currRay = scattered;
                } else {
                    return {0, 0, 0};
                }
            } else {
                const auto a = 0.5 * (normalize(currRay.dir).y + 1.0);
                return currAttenuation * jtx::lerp(Color(1, 1, 1), Color(0.5, 0.7, 1.0), a);
            }
        }

        // Exceeded bounce depth
        return {0, 0, 0};
    }
};