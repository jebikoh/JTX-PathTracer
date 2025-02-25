#pragma once

#include "image.hpp"
#include "scene.hpp"
#include "util/rand.hpp"
#include <atomic>
#include <thread>
#include <utility>

/**
 * Camera base class
 *
 * Contains common functionality between the two camera classes StaticCamera and DynamicCamera.
 *
 * Rendering logic is implemented the respective methods of the derived classes due to differences
 * in thread management and synchronization overhead.
 */
class Camera {
public:
    // Image Properties
    int width_, height_;
    Float aspectRatio_;

    // Camera Properties
    int xPixelSamples_;
    int yPixelSamples_;
    int maxDepth_;
    CameraProperties properties_;

    RGB8Image img_;
    std::atomic<int> currentSample_;

    /**
     * Constructor
     * @param width Image/Viewport width
     * @param height Image/Viewport height
     * @param cameraProperties Camera orientation and focus properties
     * @param xPixelSamples Number of sub-pixel samples in the x direction
     * @param yPixelSamples Number of sub-pixel samples in the y direction
     * @param maxDepth Maximum ray depth
     */
    explicit Camera(const int width, const int height, CameraProperties cameraProperties, const int xPixelSamples, const int yPixelSamples, const int maxDepth)
        : width_(width),
          height_(height),
          aspectRatio_(static_cast<Float>(width) / static_cast<Float>(height)),
          maxDepth_(maxDepth),
          xPixelSamples_(xPixelSamples),
          yPixelSamples_(yPixelSamples),
          img_(width, height),
          properties_(std::move(cameraProperties)),
          acc_(width, height) {}

    /**
     * Saves the image buffer to a file (.png)
     * @param path Path to save the image
     */
    void save(const char *path) const { img_.save(path); }

    /**
     * Resizes the camera viewport
     * @param w Width
     * @param h Height
     */
    void resize(int w, int h);

    /**
     * Clears the image buffer
     */
    void clear() { this->img_.clear(); }

    /**
     * Terminates the current render. Can take up to a frame to stop.
     */
    void terminateRender() { stopRender_ = true; }

    /**
     * Calculates the number of samples per pixel
     * @return Number of samples per pixel
     */
    int getSpp() const { return xPixelSamples_ * yPixelSamples_; }

protected:
    Vec3 vp00_;
    Vec3 du_, dv_;
    Vec3 u_, v_, w_;
    Vec3 defocus_u_, defocus_v_;
    AccumulationBuffer acc_;

    /**
     * Samples a point on the Camera's defocus disc
     * @param rng RNG instance
     * @return Point on the defocus disc
     */
    Vec3 sampleDefocusDisc(RNG &rng) const {
        Vec3 p = rng.sampleUnitDisc();
        return properties_.center + (p.x * defocus_u_) + (p.y * defocus_v_);
    }

    bool stopRender_ = false;

    /**
     * Recalculates viewport and other settings
     */
    void init();

    /**
     * Samples a ray from the camera
     * @param i Row
     * @param j Column
     * @param stratum Stratum (subpixel sample)
     * @param rng RNG instance
     * @return Ray
     */
    Ray getRay(const uint32_t i, const uint32_t j, const uint32_t stratum, RNG &rng) const {
        const uint32_t x = stratum % xPixelSamples_;
        const uint32_t y = stratum / xPixelSamples_;

        const float dx = rng.sample<float>();
        const float dy = rng.sample<float>();

        const auto offset = Vec2f((static_cast<float>(x) + dx) / static_cast<float>(xPixelSamples_), (static_cast<float>(y) + dy) / static_cast<float>(yPixelSamples_));
        const auto sample = vp00_ + (static_cast<float>(i) + offset.x) * du_ + (static_cast<float>(j) + offset.y) * dv_;

        auto origin = (properties_.defocusAngle <= 0) ? properties_.center : sampleDefocusDisc(rng);
        return {origin, sample - origin, rng.sample<float>()};
    }
};

/**
 * Job for a single ray tracing thread
 */
struct RayTraceJob {
    uint32_t startRow;
    uint32_t startCol;
    uint32_t endRow;
    uint32_t endCol;
};

/**
 * Work queue for ray tracing threads
 */
struct WorkQueue {
    std::vector<RayTraceJob> jobs;
    std::atomic<uint64_t> nextJobIndex;

    /**
     * Resets the work queue
     */
    void reset() { nextJobIndex = 0; }
};

/**
 * Static Camera
 *
 * This camera performs a render of a single frame per call to render().
 *
 * Uses threads and work queue allocated per call, with less synchronization overhead.
 * Meant for single frame final renders.
 */
class StaticCamera : public Camera {
public:
    using Camera::Camera;

    void render(const Scene &scene);
};