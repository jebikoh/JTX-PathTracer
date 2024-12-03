#pragma once

#include <jtxlib/math/vector.hpp>

#ifdef RT_DOUBLE_PRECISION
using Float = double;
using vec3   = jtx::Vec3d;
#else
using Float = float;
using vec3   = jtx::Vec3f;
#endif

