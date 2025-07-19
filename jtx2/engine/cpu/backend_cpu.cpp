#include <engine/cpu/backend_cpu.hpp>
#include <engine/cpu/integrator.hpp>
#include <util/color.hpp>

#include <barrier>
#include <filesystem>
#include <thread>

namespace jtx {

struct RayTraceJob {
    uint32_t startRow;
    uint32_t startCol;
    uint32_t endRow;
    uint32_t endCol;
};

struct WorkQueue {
    std::vector<RayTraceJob> jobs;
    std::atomic<size_t> nextJobIndex;

    void Reset() { nextJobIndex = 0; }
};

void BackendCPU::StartProgressiveRender() {
    LOG_INFO(RENDER, "Starting progressive rendering with CPU backend");

    // Initialize work queue
    WorkQueue q;
    q.nextJobIndex = 0;
    for (uint32_t r = 0; r < m_height; r += m_renderSettings.tileSize) {
        for (uint32_t c = 0; c < m_width; c += m_renderSettings.tileSize) {
            RayTraceJob job;
            job.startRow = r;
            job.startCol = c;
            job.endRow   = std::min(r + m_renderSettings.tileSize, m_height);
            job.endCol   = std::min(c + m_renderSettings.tileSize, m_width);
            q.jobs.push_back(job);
        }
    }

    // Initialize threads
    std::vector<std::thread> threads;
    threads.reserve(m_renderSettings.numThreads);

    // Setup synchronization
    const uint32_t spp                  = m_renderSettings.sppRow * m_renderSettings.sppCol;
    bool bTerminate                     = false;
    std::atomic<uint32_t> currentSample = 0;
    std::barrier endBarrier(m_renderSettings.numThreads, [&]() noexcept {
        currentSample.fetch_add(m_renderSettings.samplesPerPass, std::memory_order_relaxed);
        if (currentSample.load() >= spp) {
            bTerminate = true;
        } else {
            LOG_DEBUG(RENDER, "Render progress: {}%", static_cast<float>(currentSample.load()) / spp * 100.0f);
            q.Reset();
        }
    });

    // Launch threads
    PROFILE_LOCAL_START("Render progress");
    LOG_DEBUG(RENDER, "Launching {} threads", m_renderSettings.numThreads);
    for (uint32_t i = 0; i < m_renderSettings.numThreads; ++i) {
        threads.emplace_back([this, &bTerminate, &currentSample, &q, spp, &endBarrier] {
            while (true) {
                if (bTerminate) break;

                const int startingSample = currentSample.load();
                while (true) {
                    // Fetch job
                    const auto jobIndex = q.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
                    if (jobIndex >= q.jobs.size()) break;
                    const auto &job = q.jobs[jobIndex];

                    for (auto sample = startingSample; sample < std::min(startingSample + m_renderSettings.samplesPerPass, spp); ++sample) {
                        for (auto row = job.startRow; row < job.endRow; ++row) {
                            for (auto col = job.startCol; col < job.endCol; ++col) {
                                Sampler sampler(m_renderSettings.seed, row, col, sample + 1);
                                const ray r = m_camera.GetRay(row, col, sample, sampler);

                                // Integrate ray
                                // const vec3 intensity = Integrate(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                                // const vec3 intensity = IntegrateRR(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                                // const vec3 intensity = IntegrateNEE(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                                const vec3 intensity = IntegrateMIS(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);

                                // Accumulate
                                float *acc = JTX_IMAGE_PIXEL_PTR(m_accBuffer, row, col);
                                acc[0] += intensity.x;
                                acc[1] += intensity.y;
                                acc[2] += intensity.z;

                                // Progress rendering: apply post-FX and store in 8-bit image buffer
                                auto accIntensity = vec3(acc);

                                // TODO: Apply tonemapping

                                // Apply OETF
                                accIntensity = SRGBToLinear(accIntensity / static_cast<float>(sample + 1));

                                // Clamp and scale
                                accIntensity = ClampIntensity(accIntensity) * 255.999f;

                                // Store in image buffer (for progressive rendering)
                                uint8_t *img = JTX_IMAGE_PIXEL_PTR(m_imgBuffer, row, col);
                                img[0]       = static_cast<uint8_t>(accIntensity[0]);
                                img[1]       = static_cast<uint8_t>(accIntensity[1]);
                                img[2]       = static_cast<uint8_t>(accIntensity[2]);
                            }
                        }
                    }
                }
                endBarrier.arrive_and_wait();
            }
        });
    }

    // Collect threads
    for (auto &thread: threads) {
        thread.join();
    }
    PROFILE_LOCAL_LOG_TIME();
    LOG_INFO(RENDER, "Progressive rendering completed");
}

void BackendCPU::StartOfflineRender() {
    LOG_INFO(RENDER, "Starting offline rendering with CPU backend");

    // Initialize work queue
    WorkQueue q;
    q.nextJobIndex = 0;
    for (uint32_t r = 0; r < m_height; r += m_renderSettings.tileSize) {
        for (uint32_t c = 0; c < m_width; c += m_renderSettings.tileSize) {
            RayTraceJob job;
            job.startRow = r;
            job.startCol = c;
            job.endRow   = std::min(r + m_renderSettings.tileSize, m_height);
            job.endCol   = std::min(c + m_renderSettings.tileSize, m_width);
            q.jobs.push_back(job);
        }
    }

    // Initialize threads
    std::vector<std::thread> threads;
    threads.reserve(m_renderSettings.numThreads);

    // Offline rendering doesn't need synchronization per sample since there is no progressive display
    const uint32_t spp = m_renderSettings.sppRow * m_renderSettings.sppCol;

    // Launch threads
    PROFILE_LOCAL_START("Render progress");
    LOG_DEBUG(RENDER, "Launching {} threads", m_renderSettings.numThreads);
    for (uint32_t i = 0; i < m_renderSettings.numThreads; ++i) {
        threads.emplace_back([this, &q, spp] {
            while (true) {
                const auto jobIndex = q.nextJobIndex.fetch_add(1, std::memory_order_relaxed);
                if (jobIndex >= q.jobs.size()) break;
                const auto &job = q.jobs[jobIndex];

                for (auto sample = 0; sample < spp; ++sample) {
                    for (auto row = job.startRow; row < job.endRow; ++row) {
                        for (auto col = job.startCol; col < job.endCol; ++col) {
                            Sampler sampler(row, col, sample + 1);
                            const ray r = m_camera.GetRay(row, col, sample, sampler);

                            // const vec3 intensity = Integrate(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                            // const vec3 intensity = IntegrateRR(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                            // const vec3 intensity = IntegrateNEE(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);
                            const vec3 intensity = IntegrateMIS(r, *m_scene, m_bvh, m_renderSettings.maxDepth, sampler);

                            float *acc = JTX_IMAGE_PIXEL_PTR(m_accBuffer, row, col);// TODO: optimize this
                            acc[0] += intensity.x;
                            acc[1] += intensity.y;
                            acc[2] += intensity.z;
                        }
                    }
                }
            }
        });
    }

    // Collect threads
    for (auto &thread: threads) {
        thread.join();
    }

    const float fspp = static_cast<float>(spp);
    // Apply tonemapping and OETF
    for (uint32_t row = 0; row < m_height; ++row) {
        for (uint32_t col = 0; col < m_width; ++col) {
            const float *acc = JTX_IMAGE_PIXEL_PTR(m_accBuffer, row, col);
            vec3 intensity   = vec3(acc) / fspp;

            float exposure = 1.0f;
            switch (m_renderSettings.exposureType) {
                case EXPOSURE_MANUAL:
                    exposure = EV100ToExposure(m_renderSettings.EV);
                    break;
                case EXPOSURE_CAMERA: {
                    float ev100 = ComputeManualEV100(m_camera.settings.fStop, m_camera.settings.shutterSpeed, m_camera.settings.ISO);
                    ev100 -= m_renderSettings.EC;
                    exposure = EV100ToExposure(ev100);
                    break;
                }
                default:
                    LOG_FATAL(RENDER, "Unknown exposure type");
            }
            intensity *= exposure;

            switch (m_renderSettings.tonemapOp) {

                case TMO_NONE:
                    break;
                case TMO_REINHARD:
                    // Reinhard tonemapping
                    intensity = Reinhard(intensity);
                    break;

                default:
                    LOG_FATAL(RENDER, "Unknown tonemap op");
                    break;
            }

            intensity = LinearToSRGB(intensity);
            intensity = ClampIntensity(intensity) * 255.999f;

            uint8_t *img = JTX_IMAGE_PIXEL_PTR(m_imgBuffer, row, col);
            img[0]       = static_cast<uint8_t>(intensity[0]);
            img[1]       = static_cast<uint8_t>(intensity[1]);
            img[2]       = static_cast<uint8_t>(intensity[2]);
        }
    }

    PROFILE_LOCAL_LOG_TIME();
    LOG_INFO(RENDER, "Offline rendering completed");
}

JtxResult BackendCPU::SaveRenderOutput(const std::string &path) const {
    LOG_INFO(RENDER, "Saving render output to {}", path);
    return m_imgBuffer.Save(path);
}

}// namespace jtx