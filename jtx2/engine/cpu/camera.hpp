#pragma once
#include <jtx.hpp>
#include <engine/backends.hpp>
#include <util/sampling.hpp>

#include <cstdint>

namespace jtx {

/**
 * Thin lens camera class
 *
 * User is required to manually call update() after any changes to camera or relevant render settings
 */
struct ThinLensCamera {
    float width = 0;
    float height = 0;
    uint32_t sppRow = 1;
    uint32_t sppCol = 1;
    Scene::CameraSettings settings{};

    /**
     * Updates camera's viewport and focal lens. Must be called after any changes to the camera settings.
     */
    void Update() {
        const float aspectRatio = width / height;
        const float sensorHeight = settings.sensorWidth / aspectRatio;

        const float vpHeight = (sensorHeight / settings.focalLength) * settings.focalDistance;
        const float vpWidth  = vpHeight * aspectRatio;

        const vec3 w = jtx::Normalize(settings.position - settings.target);
        const vec3 u = jtx::Normalize(jtx::Cross(settings.up, w));
        const vec3 v = jtx::Cross(w, u);

        const vec3 vpU = vpWidth * u;
        const vec3 vpV = vpHeight * v;
        m_du           = vpU / width;
        m_dv           = vpV / height;

        m_anchor = settings.position - (settings.focalDistance * w) - vpU / 2 - vpV / 2 + 0.5 * (m_du + m_dv);

        if (settings.bEnableDof) {
            m_apertureRadius = (settings.focalLength / settings.fStop) * 0.5;
        } else {
            m_apertureRadius = 0.0f;
        }

        m_lensU = u;
        m_lensV = v;
    }

    ray GetRay(const uint32_t row, const uint32_t col, const uint32_t noSample, Sampler &rng) const {
        const uint32_t sx = noSample % sppRow;
        const uint32_t sy = noSample / sppRow;

        const float dx = rng.Uniform<float>();
        const float dy = rng.Uniform<float>();

        const float ox = (static_cast<float>(sx) + dx) / static_cast<float>(sppRow);
        const float oy = (static_cast<float>(sy) + dy) / static_cast<float>(sppCol);

        const vec3 sample = m_anchor + (static_cast<float>(col) + ox) * m_du + (static_cast<float>(row) + oy) * m_dv;

        // Change this when we implement DOF
        vec3 origin = settings.position;

        if (settings.bEnableDof) {
            const vec2 disc = SampleUniformDiskConcentric(rng.Uniform<vec2>());
            origin += (m_lensU * disc.x + m_lensV * disc.y) * m_apertureRadius;
        }

        return {origin, Normalize(sample - origin)};
    }
private:
    vec3 m_du{};
    vec3 m_dv{};
    vec3 m_anchor{};

    vec3 m_lensU{};
    vec3 m_lensV{};
    float m_apertureRadius = 0.0f;
};

}// namespace jtx