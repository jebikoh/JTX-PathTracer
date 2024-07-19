#pragma once

#include "rtx.hpp"

using Color = jtx::Vec3d;
static const double RGB_SCALE = 255.999;

void write_color(std::ostream &out, const Color &color) {
    int rbyte = int(RGB_SCALE * color.x);
    int gbyte = int(RGB_SCALE * color.y);
    int bbyte = int(RGB_SCALE * color.z);

    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}