#pragma once

#include "rt.hpp"

using Color = vec3;
static constexpr Float RGB_SCALE = 255.999;

void writeColor(std::ostream &out, const Color &pixelColor) {
    int r = int(RGB_SCALE * pixelColor.r);
    int g = int(RGB_SCALE * pixelColor.g);
    int b = int(RGB_SCALE * pixelColor.b);

    out << r << ' ' << g << ' ' << b << '\n';
}