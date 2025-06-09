#pragma once
#include "../jtx.hpp"

namespace jtx {

// Hashing functions taken from: https://www.jcgt.org/published/0009/03/02/
inline uint32_t xxHash32(const vec3u &p) {
    constexpr uint32_t PRIME32_2 = 2246822519u;
    constexpr uint32_t PRIME32_3 = 3266489917u;
    constexpr uint32_t PRIME32_4 = 668265263u;
    constexpr uint32_t PRIME32_5 = 374761393u;

    uint32_t h32 = p.z + PRIME32_5 + p.x * PRIME32_3;
    h32          = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 += p.y * PRIME32_3;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

// RXS-M-XS: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
inline uint32_t PCG(uint32_t &state) {
    const uint32_t s    = state;
    state               = state * 747796405u + 2891336453u;
    const uint32_t word = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (word >> 22u) ^ word;
}

}// namespace jtx