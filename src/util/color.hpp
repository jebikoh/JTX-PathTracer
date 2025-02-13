#pragma once

#include "../rt.hpp"

using Color = Vec3;
static constexpr Float RGB_SCALE = 255.999;

static const auto WHITE = Color(1, 1, 1);
static const auto BLACK = Color(0, 0, 0);
static const auto SKY_BLUE = Color(0.529, 0.808, 0.922);

DEV INLINE void writeColor(std::ostream &out, const Color &pixelColor) {
    const int r = static_cast<int>(RGB_SCALE * pixelColor.r);
    const int g = static_cast<int>(RGB_SCALE * pixelColor.g);
    const int b = static_cast<int>(RGB_SCALE * pixelColor.b);

    out << r << ' ' << g << ' ' << b << '\n';
}