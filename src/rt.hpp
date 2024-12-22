#pragma once

#include <jtxlib/math.hpp>
#include <random>


// #define USE_BVH

#ifdef USE_DOUBLE_PRECISION
using Float = double;
using Vec3  = jtx::Vec3d;
using Ray   = jtx::Rayd;
constexpr Float INF = jtx::INFINITY_D;
constexpr Float PI  = jtx::PI;
#else
using Float = float;
using Vec3  = jtx::Vec3f;
using Ray   = jtx::Rayf;
constexpr Float INF = jtx::INFINITY_F;
constexpr Float PI  = jtx::PI_F;
#endif