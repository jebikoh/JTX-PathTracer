#pragma once
#include <jtx.hpp>
#include <util/rng.hpp>
#include <backends/backends.hpp>

#include <cstdint>

namespace jtx {

struct StaticCamera {
    CameraSettings settings;
    // TODO: think of a better way to parameterize these
    uint32_t sppRow;
    uint32_t sppCol;


    void init(const uint32_t width, const uint32_t height, const uint32_t sppRow, const uint32_t sppCol) {
        m_width  = width;
        m_height = height;
        this->sppRow = sppRow;
        this->sppCol = sppCol;
    }

    /**
     * Updates camera's viewport and focal lens.
     * Must be called after any resize() operation or any updates to camera settings
     */
    void update() {
        const float aspectRatio = m_width / m_height;

        const float height   = jtx::tan(jtx::radians(settings.yfov) / 2);
        const float vpHeight = 2 * height * settings.focalDistance;
        const float vpWidth  = vpHeight * aspectRatio;

        const vec3 w = jtx::normalize(settings.position - settings.target);
        const vec3 u = jtx::normalize(jtx::cross(settings.up, w));
        const vec3 v = jtx::cross(w, u);

        const vec3 vpU = vpWidth * u;
        const vec3 vpV = vpHeight * v;
        m_du           = vpU / m_width;
        m_dv           = vpV / m_height;

        m_anchor = settings.position - (settings.focalDistance * w) - vpU / 2 - vpV / 2 + 0.5 * (m_du + m_dv);
    }

    ray getRay(const uint32_t row, const uint32_t col, const uint32_t stratum, RNG &rng) const {
        const uint32_t sx = stratum % sppRow;
        const uint32_t sy = stratum / sppCol;

        const float dx = rng.uniform<float>();
        const float dy = rng.uniform<float>();

        const float ox = (static_cast<float>(sx) + dx) / static_cast<float>(sppRow);
        const float oy = (static_cast<float>(sy) + dy) / static_cast<float>(sppCol);

        const vec3 sample = m_anchor + (static_cast<float>(col) + ox) * m_du + (static_cast<float>(row) + oy) * m_dv;

        // Change this when we implement DOF
        const auto origin = settings.bEnableDof ? settings.position : settings.position;
        return {origin, sample - origin};
    }

protected:
    float m_width = 0;
    float m_height = 0;
    float m_channels = 3;

    vec3 m_du{};
    vec3 m_dv{};
    vec3 m_anchor{};
};

}// namespace jtx