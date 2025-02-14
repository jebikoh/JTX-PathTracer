#include "camera.hpp"
#include "bvh.hpp"
#include "integrator.hpp"

#include <barrier>
#include <thread>

struct RayTraceJob {
    uint32_t startRow;
    uint32_t startCol;
    uint32_t endRow;
    uint32_t endCol;

    const Scene *scene;
    RGB8Image *img;
};
// i really like mich <3
struct WorkQueue {
    std::vector<RayTraceJob> jobs;

    std::atomic<uint64_t> totalBounces;
    std::atomic<uint64_t> nextJobIndex;
};

void Camera::render(const Scene &scene) {
    // Need to re-initialize everytime to reflect changes via UI
    init();
    stopRender_ = false;
    acc_.clear();

    // Setup work queue and work orders
    // We will create 32x32 tiles for each thread to work on
    WorkQueue queue{};
    queue.totalBounces = 0;
    queue.nextJobIndex = 0;
    for (int r = 0; r < height_; r += 32) {
        for (int c = 0; c < width_; c += 32) {
            RayTraceJob job{};
            job.scene    = &scene;
            job.img      = &img_;
            job.startRow = r;
            job.startCol = c;
            job.endRow   = std::min(r + 32, height_);
            job.endCol   = std::min(c + 32, width_);
            queue.jobs.push_back(job);
        }
    }

#ifdef ENABLE_MULTI_THREADING
    // Set up threads
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;
#else
    unsigned int threadCount = 1;
#endif

    // reset the current sample to 0
    currentSample_.store(0);

    std::barrier endBarrier(threadCount, [&]() noexcept {
        currentSample_.fetch_add(1);
        queue.nextJobIndex = 0;
    });

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    const int spp = xPixelSamples_ * yPixelSamples_;

    for (unsigned int t = 0; t < threadCount; ++t) {
        threads.emplace_back([this, &queue, &endBarrier, &spp] {
            while (true) {
                const int sample = currentSample_.load();
                if (sample >= spp || stopRender_) { break; }

                int numRays = 0;
                while (true) {
                    const auto jobIndex = queue.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
                    if (jobIndex >= queue.jobs.size()) { break; }

                    const auto &job = queue.jobs[jobIndex];

                    for (int row = job.startRow; row < job.endRow; ++row) {
                        for (int col = job.startCol; col < job.endCol; ++col) {
                            if (stopRender_) break;
                            // Seeds with FNV1-a
                            // PCG via RXS-M-XS
                            RNG sampler(row, col, sample + 1);

                            const Ray r = getRay(col, row, sample, sampler);

                            // Color sampleColor = integrateBasic(r, *job.scene, maxDepth_, sampler);
                            // Color sampleColor = integrate(r, *job.scene, maxDepth_, sampler);
                            Vec3 sampleColor = integrateMIS(r, *job.scene, maxDepth_, false, sampler);

                            // Clamp the color
                            if (sampleColor[0] > 1.0f) sampleColor[0] = 1.0f;
                            if (sampleColor[1] > 1.0f) sampleColor[1] = 1.0f;
                            if (sampleColor[2] > 1.0f) sampleColor[2] = 1.0f;

                            auto currAcc = acc_.updatePixel(sampleColor, row, col);
                            auto px      = currAcc / static_cast<float>(sample + 1);
                            img_.setPixel(currAcc / static_cast<float>(sample + 1), row, col);
                        }
                    }
                }
                queue.totalBounces.fetch_add(numRays, std::memory_order_relaxed);
                endBarrier.arrive_and_wait();
            }
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }
}

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
