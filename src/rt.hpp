#pragma once

#include <jtxlib/math/ray.hpp>
#include <jtxlib/math/vector.hpp>

#ifdef RT_DOUBLE_PRECISION
using Float = double;
using Vec3  = jtx::Vec3d;
using Ray   = jtx::Rayd;
#else
using Float = float;
using Vec3  = jtx::Vec3f;
using Ray   = jtx::Rayf;
#endif
