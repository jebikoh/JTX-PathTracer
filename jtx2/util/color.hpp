#pragma once

#include "jtx.hpp"

namespace jtx {

// -- Exposure --


// -- Tonemapping --
enum kTonemapping {
    REINHARD,
    REINHARD_EXT
};

inline float Luminance(const vec3 &color) {
    return 0.2125 * color.x + 0.7154 * color.y + 0.0721 * color.z;
}

inline vec3 ApplyLuminance(const vec3 &color, const float L1, const float L2) {
    if (L1 == 0.0f) return color;
    return color * (L2 / L1);
}

// Tonemapping
inline vec3 Reinhard(const vec3 &v) {
    const float L1 = Luminance(v);
    return v / (1.0f + L1);
}

inline vec3 ReinhardExt(const vec3 &v, float whitePoint) {
    const float L1 = Luminance(v);
    const float n = L1 * (1.0f + L1 / (whitePoint * whitePoint));
    const float d = 1 + L1;
    const float L2 = n / d;
    return ApplyLuminance(v, L1, L2);
}

inline float ApplyGamma(const float x) {
    if (x > 0) return jtx::Sqrt(x);
    return 0;
}

inline float ClampIntensity(float i) {
    return jtx::Clamp(i, 0.0f, 0.999f);
}

inline vec3 SRGBToLinear(const vec3 &srgb) {
    vec3 linear;
    for (int i = 0; i < 3; ++i) {
        if (srgb[i] <= 0.04045f) {
            linear[i] = srgb[i] / 12.92f;
        } else {
            linear[i] = jtx::pow((srgb[i] + 0.055f) / 1.055f, 2.4f);
        }
    }
    return linear;
}

namespace color {
    static const auto WHITE = vec3(1.0f, 1.0f, 1.0f);
    static const auto BLACK = vec3(0.0f, 0.0f, 0.0f);
    static const auto RED   = vec3(1.0f, 0.0f, 0.0f);
    static const auto GREEN = vec3(0.0f, 1.0f, 0.0f);
    static const auto BLUE  = vec3(0.0f, 0.0f, 1.0f);
}// namespace color

}// namespace jtx