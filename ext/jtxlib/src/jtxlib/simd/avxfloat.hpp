#pragma once
#pragma clang diagnostic push
#pragma ide diagnostic ignored "portability-simd-intrinsics"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"

#include <immintrin.h>
#include "../math/mat4.hpp"

namespace jtx {
    /**
     * AVXFloat is a small wrapper around __m256 (AVX2) to be used in a SoA data layout
     * This is based off: https://github.com/dubiousconst282/GLimpSW/blob/main/src/SwRast/SIMD.h
     * Aligned as 32 bytes (256 bits)
     */
    struct AVXFloat {
        static constexpr size_t size = sizeof(__m256) / sizeof(float);

        __m256 data;

        // Sets all floats to 0
        AVXFloat() : data(_mm256_setzero_ps()) {}
        // Copies data
        AVXFloat(__m256 data) : data(data) {}
        // Sets all floats to given value (super useful for M * V operations)
        AVXFloat(float val) {
            data = _mm256_set1_ps(val);
        }

        inline float& operator[](size_t i) {
            return reinterpret_cast<float*>(&data)[i];
        }

        inline const float& operator[](size_t i) const {
            return reinterpret_cast<const float*>(&data)[i];
        }

        // Loads 8 floats from ptr into AVXFloat
        static inline AVXFloat load(const float *ptr) {
            return _mm256_load_ps(ptr);
        }
        // Stores 8 floats from AVXFloat into ptr
        inline void store(float *ptr) const {
            _mm256_store_ps(ptr, data);
        }

        inline operator __m256() const { return data; }
    };

    inline AVXFloat operator+(AVXFloat a, AVXFloat b) {
        return _mm256_add_ps(a, b);
    }

    inline AVXFloat operator*(AVXFloat a, AVXFloat b) {
        return _mm256_mul_ps(a, b);
    }

    /**
     * AVXVec3f and AVXVec4f are SoA structs for 3D and 4D vectors
     * Each holds an AVXFloat per component, meaning they hold 8 vectors at a time
     */
    struct AVXVec4f;

    // Aligned as 32 bytes * 3 (96 bytes)
    struct AVXVec3f {
        AVXFloat x, y, z;

        AVXVec3f() = default;
        AVXVec3f(AVXFloat _x, AVXFloat _y, AVXFloat _z) : x(_x), y(_y), z(_z) {}

        AVXVec3f(float v) : x(v), y(v), z(v) {}
        AVXVec3f(AVXFloat v) : x(v), y(v), z(v) {}
        AVXVec3f(const AVXVec4f &v);
    };

    // Aligned as 32 bytes * 4 (128 bytes)
    struct AVXVec4f {
        AVXFloat x, y, z, w;

        AVXVec4f() = default;
        AVXVec4f(AVXFloat _x, AVXFloat _y, AVXFloat _z, AVXFloat _w) : x(_x), y(_y), z(_z), w(_w) {}
        AVXVec4f(AVXVec3f v, AVXFloat _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

        AVXVec4f(float v) : x(v), y(v), z(v), w(v) {}
        AVXVec4f(AVXFloat v) : x(v), y(v), z(v), w(v) {}
    };

    AVXVec3f::AVXVec3f(const AVXVec4f &v) : x(v.x), y(v.y), z(v.z) {}

    inline AVXVec4f transformVec(const AVXVec4f &v, const jtx::Mat4 &m) {
        AVXFloat x = m.data[0][0] * v.x + m.data[0][1] * v.y + m.data[0][2] * v.z + m.data[0][3] * v.w;
        AVXFloat y = m.data[1][0] * v.x + m.data[1][1] * v.y + m.data[1][2] * v.z + m.data[1][3] * v.w;
        AVXFloat z = m.data[2][0] * v.x + m.data[2][1] * v.y + m.data[2][2] * v.z + m.data[2][3] * v.w;
        AVXFloat w = m.data[3][0] * v.x + m.data[3][1] * v.y + m.data[3][2] * v.z + m.data[3][3] * v.w;
        return {x, y, z, w};
    }

    inline AVXVec3f transformNormal(const AVXVec3f &v, const jtx::Mat4&m) {
        AVXFloat x = m.data[0][0] * v.x + m.data[0][1] * v.y + m.data[0][2] * v.z;
        AVXFloat y = m.data[1][0] * v.x + m.data[1][1] * v.y + m.data[1][2] * v.z;
        AVXFloat z = m.data[2][0] * v.x + m.data[2][1] * v.y + m.data[2][2] * v.z;
        return {x, y, z};
    }
}


#pragma clang diagnostic pop
#pragma clang diagnostic pop