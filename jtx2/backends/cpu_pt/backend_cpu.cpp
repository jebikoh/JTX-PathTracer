#include "backend_cpu.hpp"

#include <barrier>
#include <thread>
#include <backends/cpu_pt/integrator.hpp>
#include <filesystem>

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

    void reset() { nextJobIndex = 0; }
    bool workAvailable() const { return nextJobIndex < jobs.size(); }
};

void BackendCPU::startRendering() {
    LOG_INFO(RENDER, "Starting rendering with CPU backend");
    // Updates camera every time; use dirty flag if needed later
    m_camera.update();

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
        }
    });

    // Launch threads
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
                                Sampler sampler(row, col, sample + 1);
                                const ray r = m_camera.getRay(row, col, sample, sampler);

                                // Integrate ray
                                // For now, we test with a simple line trace
                                vec3 intensity{};
                                TriangleIntersection isect;
                                if (m_bvh.closestHit(r, 0, JTX_INFINITY_F, isect)) {
                                    intensity = vec3(1.0f);
                                }

                                // Add to accumulation buffer
                                float *acc = JTX_IMAGE_PIXEL_PTR(m_accBuffer, row, col);
                                acc[0] += intensity.x;
                                acc[1] += intensity.y;
                                acc[2] += intensity.z;

                                // Apply tonemapping

                                // Apply OETF

                                // Store in image buffer (for progressive rendering)
                                uint8_t *img = JTX_IMAGE_PIXEL_PTR(m_imgBuffer, row, col);
                                img[0] = static_cast<uint8_t>(acc[0] / static_cast<float>(sample + 1) * 255.999);
                                img[1] = static_cast<uint8_t>(acc[1] / static_cast<float>(sample + 1) * 255.999);
                                img[2] = static_cast<uint8_t>(acc[2] / static_cast<float>(sample + 1) * 255.999);
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
    LOG_INFO(RENDER, "Rendering completed");
    // TODO: remove this
    if (m_imgBuffer.save("img.png")) {
        LOG_DEBUG(RENDER, "Output saved");
    } else {
        LOG_DEBUG(RENDER, "Output not saved");
    }
}

}// namespace jtx