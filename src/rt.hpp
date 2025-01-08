#pragma once

#include <jtxlib/math.hpp>


class BVHTree;
class PrimitiveList;

#define USE_BVH_AS_WORLD

#ifdef USE_BVH_AS_WORLD
using World = BVHTree;
#else
using World = PrimitiveList;
#endif

#ifdef USE_DOUBLE_PRECISION
using Float = double;
using Vec3  = jtx::Vec3d;
using Vec2 = jtx::Vec2d;
using Ray   = jtx::Rayd;
constexpr Float INF = jtx::INFINITY_D;
constexpr Float PI  = jtx::PI;
#else
using Float = float;
using Vec3  = jtx::Vec3f;
using Vec2 = jtx::Vec2f;
using Ray   = jtx::Rayf;
constexpr Float INF = jtx::INFINITY_F;
constexpr Float PI  = jtx::PI_F;
#endif

#ifdef ENABLE_CUDA_BACKEND
#define HOSTDEV __host__ __device__
#define DEV __device__
#define HOST __host__
#define INLINE __forceinline__
#else
#define HOSTDEV
#define DEV
#define HOST
#define INLINE inline
#endif

inline Float radians(const Float degrees) {
    return degrees * PI / static_cast<float>(180.0);
}