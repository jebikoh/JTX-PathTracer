#include "camera.hpp"

struct RayTraceJob {
    uint32_t startRow;
    uint32_t endRow;

    const World *world;
    RGBImage *img;
};
// i really like mich <3
struct WorkQueue {
    std::vector<RayTraceJob> jobs;

    volatile uint64_t nextJobIndex;
    volatile uint64_t totalBounces;
};

void Camera::render(const World &world) {
    // Need to re-initialize everytime to reflect changes via UI
    init();
    stopRender_ = false;
    std::atomic<int> numRays{0};
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
            numRays.fetch_add(localNumRays, std::memory_order_relaxed);
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
    const auto stopTime            = std::chrono::high_resolution_clock::now();
    const double renderTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(stopTime - startTime).count();
    const double renderTimeMillis  = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();

    std::cout << "Total render time: " << renderTimeSeconds << "s" << std::endl;
    std::cout << "Num rays: " << numRays.load() << std::endl;
    std::cout << " - **Mrays/s**: " << numRays.load() / 1000000.0 / renderTimeSeconds << std::endl;
    std::cout << std::fixed << " - **ms/ray**: " << renderTimeMillis / numRays << std::endl;
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

Color Camera::rayColor(const Ray &r, const World &world, const int depth, int &numRays) {
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