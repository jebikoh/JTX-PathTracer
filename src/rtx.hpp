#pragma once

#include <cmath>
#include <iostream>
#include <jtxlib/math/ray.hpp>
#include <jtxlib/math/vec3.hpp>
#include <limits>
#include <memory>
#include <random>

#include "color.hpp"
#include "interval.hpp"

using jtx::Point3d;
using jtx::Rayd;
using jtx::Vec3d;

using std::make_shared;
using std::shared_ptr;
using std::sqrt;

using jtx::INFINITY_D;
using jtx::NEG_INFINITY_D;

inline double randomDouble() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline double randomDouble(double min, double max) {
    return min + (max - min) * randomDouble();
}
