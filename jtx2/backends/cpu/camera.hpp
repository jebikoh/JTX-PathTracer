#pragma once
#include <cstdint>
#include <jtx.hpp>

namespace jtx {

// TODO: move definition of RenderSettings and CameraSettings to general backend scope
struct RenderSettings {
    int32_t width    = 1920;
    int32_t height   = 1080;
    int32_t sppRow   = 16;
    int32_t sppCol   = 16;
    int32_t maxDepth = 32;
};

// TODO: convert orientation to quaternion
struct CameraSettings {
    vec3 position{};
    vec3 target{};
    vec3 up{};

    float focalLength; // Focal length in mm

    bool bEnableDof = false; // Enable/disable depth of field
    float focalDistance = 1.0f; // Focal distance in meters
    float fStop = 2.8f; // Aperture size (f-stop)

    // TODO:
    // - Exposure (ISO100)
    // - Tone mapping: ACES, Reinhard, Uncharted2
    // - Display device: sRGB, Display P3
};

struct StaticCamera {
    RenderSettings renderSettings;
    CameraSettings cameraSettings;

    /**
     * Updates camera's viewport and focal lens.
     * Must be called after any updates to render or camera settings before rendering
     */
    void update() {
        m_aspectRatio = static_cast<float>(renderSettings.width) / static_cast<float>(renderSettings.height);

        const float height   = jtx::tan(jtx::radians(cameraSettings.yfov) / 2);
        const float vpHeight = 2 * height * cameraSettings.focalDistance;
        const float vpWidth  = vpHeight * m_aspectRatio;

        const vec3 w = jtx::normalize(cameraSettings.position - cameraSettings.target);
        const vec3 u = jtx::normalize(jtx::cross(cameraSettings.up, w));
        const vec3 v = jtx::cross(w, u);

        const vec3 vpU = vpWidth * u;
        const vec3 vpV = vpHeight * v;
        m_du           = vpU / static_cast<float>(renderSettings.width);
        m_dv           = vpV / static_cast<float>(renderSettings.height);

        m_anchor = cameraSettings.position - (cameraSettings.focalDistance * w) - vpU / 2 - vpV / 2 + 0.5 * (m_du + m_dv);
    }

protected:
    float m_aspectRatio = 1920.0f / 1080.0f;
    vec3 m_du, m_dv;
    vec3 m_anchor;
};

}// namespace jtx