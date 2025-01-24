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

    const BVHTree *world;
    RGBImage *img;
};
// i really like mich <3
struct WorkQueue {
    std::vector<RayTraceJob> jobs;

    std::atomic<uint64_t> totalBounces;
    std::atomic<uint64_t> nextJobIndex;
};

void Camera::render(const BVHTree &world) {
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
            job.world  = &world;
            job.img    = &img_;
            job.startRow = r;
            job.startCol = c;
            job.endRow = std::min(r + 32, height_);
            job.endCol = std::min(c + 32, width_);
            queue.jobs.push_back(job);
        }
    }

    // Set up threads
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;

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

                           Ray r             = getRay(col, row, sample, sampler);

                           Color sampleColor = integrate(r, *job.world, maxDepth_, properties_.background, sampler);

                           // Color sampleColor = rayColor(r, *job.world, maxDepth_, numRays, sampler);

                           auto currAcc = acc_.updatePixel(sampleColor, row, col);
                           img_.setPixel(currAcc / static_cast<float>(sample + 1), row, col);
                       }
                   }
               }
               queue.totalBounces.fetch_add(numRays, std::memory_order_relaxed);
               endBarrier.arrive_and_wait();
           }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }
}

void Camera::init() {
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

Color Camera::rayColor(const Ray &r, const BVHTree &world, const int depth, int &numRays, RNG &rng) const {
    Color aColor = {0.0f, 0.0f, 0.0f};
    Color attenuation = {1.0, 1.0, 1.0};
    Ray currRay = r;

    for (int i = 0; i < depth; ++i) {
        HitRecord record;

        // Check for any hit
        if (world.hit(currRay, Interval(0.001, INF), record)) {
            // Get emissive component
            Color emitted = record.material->emission;

            // Accumulate emitted light by current attenuation
            // Unroll the recursive function if this is confusing
            aColor += attenuation * emitted;

            Ray sRay;
            Color sAttenuation;

            // Check if we scatter
            if (scatter(record.material, currRay, record, sAttenuation, sRay, rng)) {
                // Ray is scattered: update rolling attenuation and current ray
                attenuation *= sAttenuation;
                currRay = sRay;
            } else {
                // No scatter: return accumulated color
                return aColor;
            }

        } else {
            // No hit:
            // Add background contribution and return
            aColor += attenuation * properties_.background;
            return aColor;
        }
     }

    // Depth exceeded
    return {0, 0, 0};
}