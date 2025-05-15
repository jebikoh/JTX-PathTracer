#pragma once

#ifdef JTX_SIMD_ARM_NEON
#include <arm_neon.h>
#endif

#ifdef JTX_SIMD_X86_SSE4_2
#include <nmmintrin.h>

#define JTX_SIMD_DOT(x0, y0, z0, x1, y1, z1) _mm_add_ps(_mm_add_ps(_mm_mul_ps(x0, x1), _mm_mul_ps(y0, y1)), _mm_mul_ps(z0, z1))
#endif

namespace jtx {

union vfloat4 {
    float v[4];
#ifdef JTX_SIMD_ARM_NEON
    float32x4_t v4;
#endif
#ifdef JTX_SIMD_X86_SSE4_2
    __m128 v4;
#endif
};

}// namespace jtx