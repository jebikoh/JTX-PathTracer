#include "camera.hpp"
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

    // Set up synchronization
    currentSample_.store(0);
    std::barrier endBarrier(threadCount_, [&]() noexcept {
        currentSample_.fetch_add(samplesPerPass_);
        if (currentSample_.load() >= spp_) {
            stopRender_ = true;
        }
        queue.nextJobIndex = 0;
    });

    std::vector<std::thread> threads;
    threads.reserve(threadCount_);



    spp_ = getSpp();

    for (unsigned int t = 0; t < threadCount_; ++t) {
        threads.emplace_back([this, &queue, &scene, &endBarrier] {
            while (true) {
                if (stopRender_) { break; }
                const int sample = currentSample_.load();

                while (true) {
                    const auto jobIndex = queue.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
                    if (jobIndex >= queue.jobs.size()) { break; }

                    const auto &job = queue.jobs[jobIndex];

                    for (auto currSample = sample; currSample < jtx::min(sample + samplesPerPass_, spp_); currSample++) {
                        if (stopRender_) break;
                        for (auto row = job.startRow; row < job.endRow; ++row) {
                            if (stopRender_) break;
                            for (auto col = job.startCol; col < job.endCol; ++col) {
                                if (stopRender_) break;

                                // Seed the PCG with row, column, and sample #
                                RNG sampler(row, col, sample + 1);

                                const Ray r = getRay(col, row, currSample, sampler);

                                // Color sampleColor = integrateBasic(r, *job.scene, maxDepth_, sampler);
                                // Color sampleColor = integrate(r, *job.scene, maxDepth_, sampler);
                                Vec3 sampleColor = integrateMIS(r, scene, maxDepth_, false, sampler);

                                // Clamp the color
                                if (sampleColor[0] > 1.0f) sampleColor[0] = 1.0f;
                                if (sampleColor[1] > 1.0f) sampleColor[1] = 1.0f;
                                if (sampleColor[2] > 1.0f) sampleColor[2] = 1.0f;

                                auto currAcc = acc_.updatePixel(sampleColor, row, col);
                                img_.setPixel(currAcc / static_cast<float>(currSample + 1), row, col);
                            }
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

DynamicCamera::DynamicCamera(
        const int width,
        const int height,
        CameraProperties cameraProperties,
        const int xPixelSamples,
        const int yPixelSamples,
        const int maxDepth,
        const int samplesPerPass,
        const int threadCount)
    : Camera(width, height, std::move(cameraProperties), xPixelSamples, yPixelSamples, maxDepth, threadCount),
      samplesPerPass_(samplesPerPass),
      endBarrier_(threadCount_, [&]() noexcept {
          currentSample_.fetch_add(samplesPerPass_);
          // Reset the queue if we haven't reached the spp
          if (currentSample_.load() < getSpp()) {
            queue_.reset();
          }
      }) {
    initWorkQueue();
    startThreads();
}

void DynamicCamera::initWorkQueue() {
    queue_.nextJobIndex = 0;
    queue_.jobs.clear();
    for (int r = 0; r < height_; r += 32) {
        for (int c = 0; c < width_; c += 32) {
            RayTraceJob job{};
            job.startRow = r;
            job.startCol = c;
            job.endRow   = std::min(r + 32, height_);
            job.endCol   = std::min(c + 32, width_);
            queue_.jobs.push_back(job);
        }
    }
}

void DynamicCamera::startThreads() {
    for (auto i = 0; i < threadCount_; ++i) {
        threads_.emplace_back(&DynamicCamera::workerThread, this);
    }
}

void DynamicCamera::stopThreads() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stopThreads_ = true;
    }

    queueCondition_.notify_all();
    for (auto &thread: threads_) {
        if (thread.joinable()) thread.join();
    }
}

void DynamicCamera::resize(int w, int h) {
    Camera::resize(w, h);
    initWorkQueue();
}

void DynamicCamera::render(const Scene &scene) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stopThreads_ = false;
        resetRender_ = true;
    }

    scene_ = &scene;
    init();
    acc_.clear();
    img_.clear();
    queue_.reset();
    resetRender_ = false;

    queueCondition_.notify_all();
}

void DynamicCamera::workerThread() {
    while (true) {
        // Wait for work
        std::unique_lock lock(queueMutex_);
        queueCondition_.wait(lock, [this] { return stopThreads_ || queue_.workAvailable(); });

        lock.unlock();
        if (stopThreads_) break;

        const int sample = currentSample_.load();

        while (true) {
            if (resetRender_) break;

            const auto jobIndex = queue_.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
            if (jobIndex >= queue_.jobs.size()) { break; }

            const auto &job = queue_.jobs[jobIndex];

            for (auto currSample = sample; currSample < jtx::min(sample + samplesPerPass_, getSpp()); currSample++) {
                if (resetRender_) break;
                for (auto row = job.startRow; row < job.endRow; ++row) {
                    if (resetRender_) break;
                    for (auto col = job.startCol; col < job.endCol; ++col) {
                        if (resetRender_) { break; }

                        // Seed the PCG with row, column, and sample #
                        RNG sampler(row, col, currSample + 1);

                        const Ray r = getRay(col, row, currSample, sampler);

                        // Color sampleColor = integrateBasic(r, *job.scene, maxDepth_, sampler);
                        // Color sampleColor = integrate(r, *job.scene, maxDepth_, sampler);
                        Vec3 sampleColor = integrateMIS(r, *scene_, maxDepth_, false, sampler);

                        // Clamp the color
                        if (sampleColor[0] > 1.0f) sampleColor[0] = 1.0f;
                        if (sampleColor[1] > 1.0f) sampleColor[1] = 1.0f;
                        if (sampleColor[2] > 1.0f) sampleColor[2] = 1.0f;

                        auto currAcc = acc_.updatePixel(sampleColor, row, col);
                        img_.setPixel(currAcc / static_cast<float>(currSample + 1), row, col);
                    }
                }
            }
        }// Render loop
        endBarrier_.arrive_and_wait();
    }// Thread loop
}
