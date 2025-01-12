#pragma once

#include "image.hpp"
#include "util/rand.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include <chrono>

// Update this to use PBRTv4 Camera
class Camera {
public:
    // Image properties
    int width_, height_;
    Float aspectRatio_;
    int samplesPerPx_;
    int maxDepth_;
    Color background_ = Color(0.5, 0.5, 0.5);

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

    void render(const World &world);

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


    void init();

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

    Color rayColor(const Ray &r, const World &world, const int depth, int &numRays);
};