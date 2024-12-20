#pragma once

#include "hittable.hpp"
#include "image.hpp"
#include "interval.hpp"
#include "material.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include <chrono>
#include <thread>

// Update this to use PBRTv4 Camera
class Camera {
public:
    // Image properties
    int width_, height_;
    Float aspectRatio_;
    int samplesPerPx_;
    int maxDepth_;

    RGBImage img_;

    // Camera properties
    struct Properties {
        Vec3 center;
        Vec3 target;
        Vec3 up;
        Float yfov;
        Float defocusAngle;
        Float focusDistance;
    };

    Properties properties_;

    explicit Camera(const int width, const int height, const Properties &cameraProperties, const int samplesPerPx, const int maxDepth)
        : width_(width),
          height_(height),
          aspectRatio_(static_cast<Float>(width) / static_cast<Float>(height)),
          samplesPerPx_(samplesPerPx),
          maxDepth_(maxDepth),
          img_(width, height),
          properties_(cameraProperties) {}

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
        this->width_           = width;
        this->height_          = height;
        this->aspectRatio_     = static_cast<Float>(width) / static_cast<Float>(height);
        this->properties_.yfov = yfov;
        this->samplesPerPx_    = samplesPerPx;
        this->maxDepth_        = maxDepth;

        this->properties_.center        = position;
        this->properties_.target        = target;
        this->properties_.up            = up;
        this->properties_.defocusAngle  = defocusAngle;
        this->properties_.focusDistance = focusDistance;

        img_ = RGBImage(width, height);
    }

    void render(const BVHNode &world) {
        // Need to re-initialize everytime to reflect changes via UI
        init();
        stopRender_ = false;
        int numRays = 0;
#ifdef ENABLE_MULTITHREADING
        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;

        const auto rowsPerThread = height_ / threadCount;

        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        int startRow = 0;

        const auto startTime = std::chrono::high_resolution_clock::now();

        for (unsigned int t = 0; t < threadCount; ++t) {
            int endRow = (t == threadCount - 1) ? height_ : startRow + rowsPerThread;
            threads.emplace_back([this, startRow, endRow, &world, &numRays]() {
                int localNumRays = 0;
                for (int j = startRow; j < endRow; ++j) {
                    for (int i = 0; i < width_; ++i) {
                        if (stopRender_) return;
                        auto pxColor = Color(0, 0, 0);
                        for (int s = 0; s < samplesPerPx_; ++s) {
                            Ray r = getRay(i, j);
                            pxColor += rayColor(r, world, maxDepth_, localNumRays);
                        }
                        img_.writePixel(pxColor * pxSampleScale_, j, i);
                    }
                }
                numRays += localNumRays;
            });

            startRow = endRow;
        }

        for (auto &t: threads) {
            t.join();
        }
#else
        const auto startTime = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < height_; ++j) {
            for (int i = 0; i < width_; ++i) {
                // ReSharper disable once CppDFAConstantConditions
                // ReSharper disable once CppDFAUnreachableCode
                if (stopRender_) return;
                auto pxColor = Color(0, 0, 0);
                for (int s = 0; s < samplesPerPx_; ++s) {
                    Ray r = getRay(i, j);
                    pxColor += rayColor(r, world, maxDepth_, numRays);
                }
                img_.writePixel(pxColor * pxSampleScale_, j, i);
            }
        }
#endif
        const auto stopTime = std::chrono::high_resolution_clock::now();
        const double renderTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(stopTime - startTime).count();
        const double renderTimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();

        std::cout << "Total render time: " << renderTimeSeconds << "s" << std::endl;
        std::cout << "Num rays: " << numRays << std::endl;
        std::cout << " - **Mrays/s**: " << numRays / 1000000.0 /renderTimeSeconds << std::endl;
        std::cout << " - **ms/ray**: " << renderTimeMillis / numRays << std::endl;
    }

    void save(const char *path) const {
        img_.save(path);
    }

    void resize(const int w, const int h) {
        this->width_       = w;
        this->height_      = h;
        this->aspectRatio_ = static_cast<Float>(w) / static_cast<Float>(h);

        this->img_.clear();
        this->img_.resize(w, h);
    }

    void clear() {
        this->img_.clear();
    }

    void terminateRender() {
        stopRender_ = true;
    }

    void updateCameraProperties(const Properties &properties) {
        properties_ = properties;
    }

private:
    Float pxSampleScale_;
    Vec3 vp00_;
    Vec3 du_;
    Vec3 dv_;
    Vec3 u_, v_, w_;
    Vec3 defocus_u_;
    Vec3 defocus_v_;

    bool stopRender_ = false;


    void init() {
        pxSampleScale_ = static_cast<Float>(1.0) / static_cast<Float>(samplesPerPx_);

        // Viewport dimensions
        const Float h              = jtx::tan(radians(properties_.yfov) / 2);
        const Float viewportHeight = 2 * h * properties_.focusDistance;
        const Float viewportWidth  = viewportHeight * aspectRatio_;

        w_ = normalize(properties_.center - properties_.target);
        u_ = normalize(jtx::cross(properties_.up, w_));
        v_ = jtx::cross(w_, u_);

        // Viewport offsets
        const auto viewportU = viewportWidth * u_;
        const auto viewportV = viewportHeight * v_;
        du_                  = viewportU / width_;
        dv_                  = viewportV / height_;

        // Viewport anchors
        const auto vpUpperLeft = properties_.center - (properties_.focusDistance * w_) - viewportU / 2 - viewportV / 2;
        vp00_                  = vpUpperLeft + 0.5 * (du_ + dv_);

        // Defocus disk
        const Float defocusRadius = properties_.focusDistance * jtx::tan(radians(properties_.defocusAngle / 2));
        defocus_u_                = defocusRadius * u_;
        defocus_v_                = defocusRadius * v_;
    }

    [[nodiscard]] Ray getRay(const int i, const int j) const {
        const auto offset = sampleSquare();
        const auto sample = vp00_ + ((i + offset.x) * du_) + ((j + offset.y) * dv_);

        auto origin = (properties_.defocusAngle <= 0) ? properties_.center : sampleDefocusDisc();
        return {origin, sample - origin, randomFloat()};
    }

    static Vec3 sampleSquare() {
        return {randomFloat() - static_cast<Float>(0.5), randomFloat() - static_cast<Float>(0.5), 0};
    }

    [[nodiscard]] Vec3 sampleDefocusDisc() const {
        Vec3 p = randomInUnitDisk();
        return properties_.center + (p.x * defocus_u_) + (p.y * defocus_v_);
    }

    static Color rayColor(const Ray &r, const BVHNode &world, const int depth, int &numRays) {
        Ray currRay           = r;
        Color currAttenuation = {1.0, 1.0, 1.0};
        for (int i = 0; i < depth; ++i) {
            ++numRays;
            HitRecord record;
            if (world.hit(currRay, Interval(0.001, INF), record)) {
                Ray scattered;
                Color attenuation;
                if (scatter(record.material,currRay, record, attenuation, scattered)) {
                    currAttenuation *= attenuation;
                    currRay = scattered;
                } else {
                    return {0, 0, 0};
                }
            } else {
                const Float a = 0.5 * (normalize(currRay.dir).y + 1.0);
                return currAttenuation * jtx::lerp(Color(1, 1, 1), Color(0.5, 0.7, 1.0), a);
            }
        }

        // Exceeded bounce depth
        return {0, 0, 0};
    }
};