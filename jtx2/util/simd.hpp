#pragma once

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace jtx {

union vfloat4 {
    float v[4];
#ifdef __ARM_NEON
    float32x4_t v4;
#endif
};

}// namespace jtx