#pragma once

// Global includes
#include <jtxlib/math.hpp>
#include <util/logger.hpp>
#include <util/timer.hpp>

// Force inline (use sparingly)
#if defined(__clang__)
#define JTX_FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline
#elif defined(__GNUC__)
#define JTX_FORCE_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define JTX_FORCE_INLINE __forceinline
#else
#error Unsupported compiler
#endif

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

const vec3 JTX_VEC3_ORIGIN = { 0.0f, 0.0f, 0.0f };

enum JtxResult {
    JTX_SUCCESS                      = 1, // Success
    JTX_FAILURE                      = 0, // Generic failure
    JTX_ERROR_INVALID_FILE_EXTENSION = -1,// Invalid file extension
    JTX_ERROR_INVALID_DATA           = -2,// Invalid data (empty or nullptr)
    JTX_ERROR_FILE_LOADING           = -3,// Error while loading file
    JTX_ERROR_FILE_INVALID_DATA      = -4,// File data is invalid
    JTX_ERROR_FILE_WRITE             = -5,// Error while writing file
};

inline const char* string_JtxResult(const JtxResult input_value) {
    switch (input_value) {
        case JTX_SUCCESS:                      return "JTX_SUCCESS";
        case JTX_FAILURE:                      return "JTX_FAILURE";
        case JTX_ERROR_INVALID_FILE_EXTENSION: return "JTX_ERROR_INVALID_FILE_EXTENSION";
        case JTX_ERROR_INVALID_DATA:           return "JTX_ERROR_INVALID_DATA";
        case JTX_ERROR_FILE_LOADING:           return "JTX_ERROR_FILE_LOADING";
        case JTX_ERROR_FILE_INVALID_DATA:      return "JTX_ERROR_FILE_INVALID_DATA";
        case JTX_ERROR_FILE_WRITE:             return "JTX_ERROR_FILE_WRITE";
        default:                               return "Unknown JtxResult value";
    }
}

inline void CheckJtxError(const JtxResult result, char const *const func, const char *const file, int const line) {
    if (result <= 0) {
        LOG_FATAL(VKE, "Detected JTX error at {}:{} '{}': {}", file, line, func, string_JtxResult(result));
    }
}

#define CHECK_JTX(err) CheckJtxError((err), #err, __FILE__, __LINE__)

constexpr float PI_OVER_4 = jtx::JTX_PI_F / 4;
constexpr float PI_OVER_2 = jtx::JTX_PI_F / 2;
constexpr float INV_TWO_PI = 1.0f / (2.0f * jtx::JTX_PI_F);
constexpr float TWO_PI = 2.0F * jtx::JTX_PI_F;
constexpr float INV_PI = 1.0f / jtx::JTX_PI_F;
constexpr float INV_4_PI = 1.0f / (4.0f * jtx::JTX_PI_F);
