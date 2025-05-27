#pragma once

#ifdef JTX_SIMD_ARM_NEON
#include <arm_neon.h>

#define JTX_SIMD_DOT(x0, y0, z0, x1, y1, z1) vaddq_f32(vaddq_f32(vmulq_f32(x0, x1), vmulq_f32(y0, y1)), vmulq_f32(z0, z1))
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

// #ifdef JTX_SIMD_ARM_NEON
// inline float32x4_t EPSILON4    = vdupq_n_f32(1e-8);
// inline float32x4_t NEG_EPSILON = vdupq_n_f32(-1e-8);
// inline float32x4_t NEG_ONE     = vdupq_n_f32(-1.0f);
// inline float32x4_t ONE         = vdupq_n_f32(1.0f);
// inline float32x4_t ZERO        = vdupq_n_f32(0.0f);
// inline float32x4_t INFINITY_4  = vdupq_n_f32(JTX_INFINITY_F);
// #endif

}// namespace jtx