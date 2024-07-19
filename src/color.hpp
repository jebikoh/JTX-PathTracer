#pragma once

#include "interval.hpp"
#include "rtx.hpp"

using Color                   = jtx::Vec3d;
static const double RGB_SCALE = 255.999;

void writeColor(std::ostream &out, const Color &color) {
    static const Interval intensity(0.000, 0.999);
    int rbyte = int(RGB_SCALE * intensity.clamp(color.x));
    int gbyte = int(RGB_SCALE * intensity.clamp(color.y));
    int bbyte = int(RGB_SCALE * intensity.clamp(color.z));

    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}