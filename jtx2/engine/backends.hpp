#pragma once

#include "util/color.hpp"


#include <jtx.hpp>

namespace jtx {

constexpr uint32_t JTX_NUM_PT_BACKENDS = 1;
enum RenderBackend {
    JTX_RENDER_BACKEND_CPU = 0
};

constexpr uint32_t JTX_NUM_VIEWPORT_BACKENDS = 1;
enum ViewportBackend {
    JTX_VIEWPORT_BACKEND_VULKAN = 0
};

/**
 * Data specific to the rendering process
 */
struct RenderSettings {
    uint32_t sppRow              = 16;
    uint32_t sppCol              = 16;
    uint32_t maxDepth            = 32;
    uint32_t tileSize            = 32;
    uint32_t numThreads          = 16;
    uint32_t samplesPerPass      = 1;
    uint32_t seed                = 0;
    kExposureType exposureType = EXPOSURE_MANUAL;
    float EV                     = 0.0f;// Manual exposure value in EV100
    float EC                     = 0.0f;// Exposure compensation
    kTonemapOp tonemapOp               = TMO_NONE;
};

}// namespace jtx