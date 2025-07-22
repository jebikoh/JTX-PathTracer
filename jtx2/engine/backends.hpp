#pragma once

#include "util/color.hpp"


#include <jtx.hpp>

namespace jtx {

enum RenderBackend {
    JTX_RENDER_BACKEND_VULKAN = 0,
    JTX_NUM_RENDER_BACKENDS
};

enum ViewportBackend {
    JTX_VIEWPORT_BACKEND_VULKAN = 0,
    JTX_NUM_VIEWPORT_BACKENDS
};

/**
 * Data specific to the rendering process
 */
struct RenderSettings {
    uint32_t width             = 1920;
    uint32_t height            = 1080;
    uint32_t sppRow            = 16;
    uint32_t sppCol            = 16;
    uint32_t spp               = 4096; // TEMPORARY
    uint32_t maxDepth          = 32;
    uint32_t tileSize          = 32;
    uint32_t numThreads        = 16; // These are CPU specific, should be moved
    uint32_t samplesPerPass    = 16;  // ^
    uint32_t seed              = 0;
    kExposureType exposureMode = EXPOSURE_MANUAL;
    float EV                   = 0.0f;// Manual exposure value in EV100
    float EC                   = 0.0f;// Exposure compensation
    kTonemapOp tonemapOp       = TMO_ACES;
    float directClamping       = 0.0f;
    float indirectClamping     = 10.0f;
};

}// namespace jtx