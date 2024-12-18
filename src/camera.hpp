#pragma once

#include "hittable.hpp"
#include "image.hpp"
#include "interval.hpp"
#include "material.hpp"

#include <thread>

// Update this to use PBRv4 Camera
class Camera {
public:
    // Image properties
    int width, height;
    Float aspectRatio;
    int samplesPerPx;
    int maxDepth;

    RGBImage img;

    // Camera properties
    Vec3 center;
    Vec3 target;
    Vec3 up;
    Float yfov;
    Float defocusAngle;
    Float focusDistance;

    explicit Camera(
            const int width,
            const int height,
            const Float yfov,
            const Vec3 &position,
            const Vec3 &target,
            const Vec3 &up,
            const Float defocusAngle,
            const Float focusDistance,
            const int samplesPerPx,
            const int maxDepth) {
        // No initializer list cuz its long and ugly to read here
        this->width        = width;
        this->height       = height;
        this->aspectRatio  = static_cast<Float>(width) / static_cast<Float>(height);
        this->yfov         = yfov;
        this->samplesPerPx = samplesPerPx;
        this->maxDepth     = maxDepth;

        this->center        = position;
        this->target        = target;
        this->up            = up;
        this->defocusAngle  = defocusAngle;
        this->focusDistance = focusDistance;

        img           = RGBImage(width, height);
    }

    void render(const HittableList &world) {
        // Need to re-initialize everytime to reflect changes via UI
        init();
        stopRender = false;

        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;

        const auto rowsPerThread = height / threadCount;

        std::cout << "Using " << threadCount << " threads" << std::endl;
        std::cout << "Rows per thread: " << rowsPerThread << std::endl;

        std::vector<std::thread> threads;
        threads.reserve(threadCount);

        int startRow = 0;
        for (unsigned int t = 0; t < threadCount; ++t) {
            int endRow = (t == threadCount - 1) ? height : startRow + rowsPerThread;
            threads.emplace_back([this, startRow, endRow, &world]() {
                for (int j = startRow; j < endRow; ++j) {
                    for (int i = 0; i < width; ++i) {
                        if (stopRender) return;
                        auto pxColor = Color(0, 0, 0);
                        for (int s = 0; s < samplesPerPx; ++s) {
                            Ray r = getRay(i, j);
                            pxColor += rayColor(r, world, maxDepth);
                        }
                        img.writePixel(pxColor * pxSampleScale, j, i);
                    }
                }
            });

            startRow = endRow;
        }

        for (auto &t: threads) {
            t.join();
        }
    }

    void save(const char *path) const {
        img.save(path);
    }

    void resize(const int w, const int h) {
        this->width = w;
        this->height = h;
        this->aspectRatio = static_cast<Float>(w) / static_cast<Float>(h);

        this->img.clear();
        this->img.resize(w, h);
    }

    void clear() {
        this->img.clear();
    }

    void terminateRender() {
        stopRender = true;
    }

private:
    Float pxSampleScale;
    Vec3 vp00;
    Vec3 du;
    Vec3 dv;
    Vec3 u, v, w;
    Vec3 defocus_u;
    Vec3 defocus_v;

    bool stopRender = false;


    void init() {
        pxSampleScale = static_cast<Float>(1.0) / static_cast<Float>(samplesPerPx);

        // Viewport dimensions
        const Float h              = jtx::tan(radians(yfov) / 2);
        const Float viewportHeight = 2 * h * focusDistance;
        const Float viewportWidth  = viewportHeight * aspectRatio;

        w = normalize(center - target);
        u = normalize(jtx::cross(up, w));
        v = jtx::cross(w, u);

        // Viewport offsets
        const auto viewportU = viewportWidth * u;
        const auto viewportV = viewportHeight * v;
        du                   = viewportU / width;
        dv                   = viewportV / height;

        // Viewport anchors
        const auto vpUpperLeft = center - (focusDistance * w) - viewportU / 2 - viewportV / 2;
        vp00                   = vpUpperLeft + 0.5 * (du + dv);

        // Defocus disk
        const Float defocusRadius = focusDistance * jtx::tan(radians(defocusAngle / 2));
        defocus_u                 = defocusRadius * u;
        defocus_v                 = defocusRadius * v;
    }

    [[nodiscard]] Ray getRay(const int i, const int j) const {
        const auto offset = sampleSquare();
        const auto sample = vp00 + ((i + offset.x) * du) + ((j + offset.y) * dv);

        auto origin = (defocusAngle <= 0) ? center : sampleDefocusDisc();
        return {origin, sample - origin, randomFloat()};
    }

    static Vec3 sampleSquare() {
        return {randomFloat() - static_cast<Float>(0.5), randomFloat() - static_cast<Float>(0.5), 0};
    }

    [[nodiscard]] Vec3 sampleDefocusDisc() const {
        Vec3 p = randomInUnitDisk();
        return center + (p.x * defocus_u) + (p.y * defocus_v);
    }

    static Color rayColor(const Ray &r, const HittableList &world, int depth) {
        Ray currRay           = r;
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