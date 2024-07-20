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
#include "rand.hpp"

using jtx::Point3d;
using jtx::Rayd;
using jtx::Vec3d;

using std::fabs;
using std::make_shared;
using std::shared_ptr;
using std::sqrt;

using jtx::INFINITY_D;
using jtx::NEG_INFINITY_D;

bool nearZero(const Vec3d &v) {
    auto s = 1e-8;
    return (fabs(v.x) < s) && (fabs(v.y) < s) && (fabs(v.z) < s);
}
