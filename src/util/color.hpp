#pragma once

#include "../rt.hpp"

static constexpr Float RGB_SCALE = 255.999;

namespace Color {
static const auto WHITE = Vec3(1, 1, 1);
static const auto BLACK = Vec3(0, 0, 0);
static const auto SKY_BLUE = Vec3(0.529, 0.808, 0.922);
};

inline Vec3 sRGBToLinear(const Vec3 &srgb) {
    Vec3 linear;
    for (int i = 0; i < 3; ++i) {
        if (srgb[i] <= 0.04045f) {
            linear[i] = srgb[i] / 12.92f;
        } else {
            linear[i] = jtx::pow((srgb[i] + 0.055f) / 1.055f, 2.4f);
        }
    }
    return linear;
}
