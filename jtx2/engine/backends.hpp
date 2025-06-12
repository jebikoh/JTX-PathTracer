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

/**
 * Data specific to camera settings
 */
struct CameraSettings {
    vec3 position{};
    vec3 target{};
    vec3 up{};

    float yfov;       // FOV for reference
    float focalLength;// Focal length in mm
    float sensorWidth;// Sensor width in mm
    // Sensor height is derived from aspect ratio

    bool bEnableDof     = false;// Enable/disable depth of field
    float focalDistance = 1.0f; // Focal distance in meters
    float fStop         = 2.8f; // Aperture size (f-stop)

    // TODO:
    // - Exposure (ISO100)
    // - Tone mapping: ACES, Reinhard, Uncharted2
    // - Display device: sRGB, Display P3
};

}// namespace jtx