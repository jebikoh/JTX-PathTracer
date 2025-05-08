#pragma once

// Global includes
#include <jtxlib/math.hpp>
#include <util/logger.hpp>
#include <util/timer.hpp>

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
using quat = jtx::Quaternion;

using ray       = jtx::Rayf;
using Transform = jtx::Transform;

// Constants
constexpr int JTX_MAX_FRAMES_IN_FLIGHT = 2;
constexpr float TWO_PI = jtx::JTX_PI_F * 2.0f;

const vec3 JTX_VEC3_ORIGIN = { 0.0f, 0.0f, 0.0f };

enum JtxResult {
    JTX_SUCCESS                      = 0, // Success
    JTX_FAILURE                      = -1,// Generic failure
    JTX_ERROR_INVALID_FILE_EXTENSION = -2,// Invalid file extension
    JTX_ERROR_INVALID_DATA           = -3,// Invalid data (empty or nullptr)
    JTX_ERROR_FILE_LOADING           = -4,// Error while loading file
    JTX_ERROR_FILE_INVALID_DATA      = -5,// File data is invalid
};
