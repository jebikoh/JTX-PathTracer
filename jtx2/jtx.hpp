#pragma once

// Global includes
#include <jtxlib/math.hpp>
#include <util/logger.hpp>

// Typedefs
using vec2  = jtx::Vec2f;
using vec2i = jtx::Vec2i;
using vec2u = jtx::Vec2<uint32_t>;

using vec3  = jtx::Vec3f;
using vec3i = jtx::Vec3i;
using vec3u = jtx::Vec3<uint32_t>;

using vec4  = jtx::Vec4f;
using vec4i = jtx::Vec4i;
using vec4u = jtx::Vec4<uint32_t>;

using mat4 = jtx::Mat4;

using Ray       = jtx::Rayf;
using Transform = jtx::Transform;

constexpr int JTX_MAX_FRAMES_IN_FLIGHT = 2;
