#pragma once

#include "jtx.hpp"

namespace jtx {

inline float linearToGamma(const float x) {
    if (x > 0) return jtx::sqrt(x);
    return 0;
}

inline float clampIntensity(float i) {
    return jtx::clamp(i, 0.0f, 0.999f);
}

inline vec3 sRGBToLinear(const vec3 &srgb) {
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