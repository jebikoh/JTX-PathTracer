#pragma once

#include "jtx.hpp"

namespace jtx {

// -- Exposure --
enum kExposureType {
    EXPOSURE_MANUAL = 0,
    EXPOSURE_CAMERA = 1,
};

// https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
inline float ComputeManualEV100(const float aperture, const float shutterSpeed, const float ISO) {
    return log2(Sqr(aperture) / shutterSpeed * 100 / ISO);
}

inline float EV100ToExposure(const float EV100) {
    return 1.2f * jtx::pow(2.0f, EV100);
}

// -- Tonemapping --
enum kTonemapOp {
    TMO_NONE = 0,
    TMO_REINHARD = 1,
    TMO_ACES = 2,
    TMO_AGX = 3,
	TMO_HABLE = 4
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

inline vec3 ReinhardExt(const vec3 &v, const float whitePoint) {
    const float L1 = Luminance(v);
    const float n = L1 * (1.0f + L1 / (whitePoint * whitePoint));
    const float d = 1 + L1;
    const float L2 = n / d;
    return ApplyLuminance(v, L1, L2);
}

inline float ApplyGamma(const float x) {
    if (x > 0) return jtx::Sqrt(x);
    return 0.0f;
}

inline vec3 ApplyGamma(const vec3 &x) {
    return vec3(ApplyGamma(x.x), ApplyGamma(x.y), ApplyGamma(x.z));
}


inline float ClampIntensity(const float x) {
    return jtx::Clamp(x, 0.0f, 0.999f);
}

inline vec3 ClampIntensity(const vec3 &x) {
    return vec3(ClampIntensity(x.x), ClampIntensity(x.y), ClampIntensity(x.z));
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

inline vec3 LinearToSRGB(const vec3 &linear) {
    vec3 srgb;
    for (int i = 0; i < 3; ++i) {
        if (linear[i] <= 0.0031308) {
            srgb[i] = 12.92f * linear[i];
        } else {
            srgb[i] = 1.055 * jtx::pow(linear[i], 1.0f / 2.4f) - 0.055f;
        }
    }
    return srgb;
}

namespace color {
    static const auto WHITE = vec3(1.0f, 1.0f, 1.0f);
    static const auto BLACK = vec3(0.0f, 0.0f, 0.0f);
    static const auto RED   = vec3(1.0f, 0.0f, 0.0f);
    static const auto GREEN = vec3(0.0f, 1.0f, 0.0f);
    static const auto BLUE  = vec3(0.0f, 0.0f, 1.0f);
}// namespace color

}// namespace jtx