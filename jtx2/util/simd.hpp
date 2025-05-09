#pragma once

#ifdef JTX_SIMD_ARM_NEON
#include <arm_neon.h>
#endif

#ifdef JTX_SIMD_X86_SSE4_2
#include <nmmintrin.h>
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