#include "camera.hpp"
#include "bvh.hpp"
#include "integrator.hpp"

#include <barrier>
#include <thread>

void Camera::init() {
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
    du_                  = viewportU / static_cast<float>(width_);
    dv_                  = viewportV / static_cast<float>(height_);

    // Viewport anchors
    const auto vpUpperLeft = properties_.center - (properties_.focusDistance * w_) - viewportU / 2 - viewportV / 2;
    vp00_                  = vpUpperLeft + 0.5 * (du_ + dv_);

    // Defocus disk
    const Float defocusRadius = properties_.focusDistance * jtx::tan(radians(properties_.defocusAngle / 2));
    defocus_u_                = defocusRadius * u_;
    defocus_v_                = defocusRadius * v_;
}

void Camera::resize(const int w, const int h) {
    this->width_       = w;
    this->height_      = h;
    this->aspectRatio_ = static_cast<Float>(w) / static_cast<Float>(h);

    this->img_.clear();
    this->img_.resize(w, h);

    this->acc_.clear();
    this->acc_.resize(w, h);
}

void StaticCamera::render(const Scene &scene) {
    // Need to re-initialize everytime to reflect and potential changes in the scene
    init();
    stopRender_ = false;
    acc_.clear();

    // Setup work queue and work orders
    // We will create 32x32 tiles for each thread to work on
    WorkQueue queue{};
    queue.nextJobIndex = 0;
    for (int r = 0; r < height_; r += 32) {
        for (int c = 0; c < width_; c += 32) {
            RayTraceJob job{};
            job.startRow = r;
            job.startCol = c;
            job.endRow   = std::min(r + 32, height_);
            job.endCol   = std::min(c + 32, width_);
            queue.jobs.push_back(job);
        }
    }

    // Set up threads
#ifdef ENABLE_MULTI_THREADING
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;
#else
    unsigned int threadCount = 1;
#endif

    // Set up synchronization
    currentSample_.store(0);
    std::barrier endBarrier(threadCount, [&]() noexcept {
        currentSample_.fetch_add(1);
        queue.nextJobIndex = 0;
    });

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    const int spp = xPixelSamples_ * yPixelSamples_;

    for (unsigned int t = 0; t < threadCount; ++t) {
        threads.emplace_back([this, &queue, &scene, &endBarrier, &spp] {
            while (true) {
                const int sample = currentSample_.load();
                if (sample >= spp || stopRender_) { break; }

                while (true) {
                    const auto jobIndex = queue.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
                    if (jobIndex >= queue.jobs.size()) { break; }

                    const auto &job = queue.jobs[jobIndex];

                    for (auto row = job.startRow; row < job.endRow; ++row) {
                        for (auto col = job.startCol; col < job.endCol; ++col) {
                            if (stopRender_) break;

                            // Seed the PCG with row, column, and sample #
                            RNG sampler(row, col, sample + 1);

                            const Ray r = getRay(col, row, sample, sampler);

                            // Color sampleColor = integrateBasic(r, *job.scene, maxDepth_, sampler);
                            // Color sampleColor = integrate(r, *job.scene, maxDepth_, sampler);
                            Vec3 sampleColor = integrateMIS(r, scene, maxDepth_, false, sampler);

                            // Clamp the color
                            if (sampleColor[0] > 1.0f) sampleColor[0] = 1.0f;
                            if (sampleColor[1] > 1.0f) sampleColor[1] = 1.0f;
                            if (sampleColor[2] > 1.0f) sampleColor[2] = 1.0f;

                            auto currAcc = acc_.updatePixel(sampleColor, row, col);
                            img_.setPixel(currAcc / static_cast<float>(sample + 1), row, col);
                        }
                    }
                }
                endBarrier.arrive_and_wait();
            }
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }
}
