#pragma once

#include <jtxlib/math.hpp>
#include <iostream>
#include <limits>
#include <memory>

#ifdef RT_DOUBLE_PRECISION
using Float = double;
using Vec3  = jtx::Vec3d;
using Ray   = jtx::Rayd;
constexpr float INF = jtx::INFINITY_D;
constexpr float PI  = jtx::PI;
#else
using Float = float;
using Vec3  = jtx::Vec3f;
using Ray   = jtx::Rayf;
constexpr float INF = jtx::INFINITY_F;
constexpr float PI  = jtx::PI_F;
#endif

inline Float radians(const Float degrees) {
    return degrees * PI / static_cast<float>(180.0);
}

#include "color.hpp"
#include "interval.hpp"