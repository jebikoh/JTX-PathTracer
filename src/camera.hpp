#pragma once

#include "image.hpp"
#include "util/rand.hpp"
#include <atomic>
#include <thread>
#include "scene.hpp"

// Update this to use PBRTv4 Camera
class Camera {
public:
    // Image properties
    int width_, height_;
    Float aspectRatio_;
    int xPixelSamples_;
    int yPixelSamples_;
    int maxDepth_;

    RGB8Image img_;

    CameraProperties properties_;

    std::atomic<int> currentSample_;

    explicit Camera(const int width, const int height, const CameraProperties &cameraProperties, const int xPixelSamples, const int yPixelSamples, const int maxDepth)
        : width_(width),
          height_(height),
          aspectRatio_(static_cast<Float>(width) / static_cast<Float>(height)),
          maxDepth_(maxDepth),
          xPixelSamples_(xPixelSamples),
          yPixelSamples_(yPixelSamples),
          img_(width, height),
          properties_(cameraProperties),
          acc_(width, height) {}

    void render(const Scene &scene);

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

    void updateCameraProperties(const CameraProperties &properties) {
        properties_ = properties;
    }

    int getSpp() const {
        return xPixelSamples_ * yPixelSamples_;
    }

private:
    Vec3 vp00_;
    Vec3 du_;
    Vec3 dv_;
    Vec3 u_, v_, w_;
    Vec3 defocus_u_;
    Vec3 defocus_v_;

    bool stopRender_ = false;

    // Similar to img, but stores floats
    // Accumulate here, then divide by sample # for img_
    AccumulationBuffer acc_;

    /**
     * Recalculates viewport and other settings
     */
    void init();

    [[nodiscard]] Ray getRay(const int i, const int j, const int stratum, RNG &rng) const {
        // Find stratum of pixel (j, i)
        const int x = stratum % xPixelSamples_;
        const int y = stratum / xPixelSamples_;

        const float dx = rng.sample<float>();
        const float dy = rng.sample<float>();

        const auto offset = Vec2f((x + dx) / xPixelSamples_, (y + dy) / yPixelSamples_);
        const auto sample = vp00_ + (i + offset.x) * du_ + (j + offset.y) * dv_;

        auto origin = (properties_.defocusAngle <= 0) ? properties_.center : sampleDefocusDisc(rng);
        return {origin, sample - origin, rng.sample<float>()};
    }

    [[nodiscard]] Vec3 sampleDefocusDisc(RNG &rng) const {
        Vec3 p = rng.sampleUnitDisc();
        return properties_.center + (p.x * defocus_u_) + (p.y * defocus_v_);
    }
};