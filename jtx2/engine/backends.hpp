#pragma once

#include <jtx.hpp>

namespace jtx {

constexpr uint32_t JTX_NUM_PT_BACKENDS = 1;
enum RenderBackend {
    JTX_RENDER_BACKEND_CPU = 0
};

constexpr uint32_t JTX_NUM_VIEWPORT_BACKENDS = 1;
enum ViewportBackend {
    JTX_VIEWPORT_BACKEND_WING = 0
};

/**
 * Data specific to the rendering process
 */
struct RenderSettings {
    uint32_t sppRow         = 16;
    uint32_t sppCol         = 16;
    uint32_t maxDepth       = 32;
    uint32_t tileSize       = 32;
    uint32_t numThreads     = 16;
    uint32_t samplesPerPass = 1;
};

}// namespace jtx