#pragma once
#include "jtx.hpp"

namespace jtx {

constexpr uint32_t JTX_XXHASH32_PRIME32_2 = 2246822519u;
constexpr uint32_t JTX_XXHASH32_PRIME32_3 = 3266489917u;
constexpr uint32_t JTX_XXHASH32_PRIME32_4 = 668265263u;
constexpr uint32_t JTX_XXHASH32_PRIME32_5 = 374761393u;

// Hashing functions taken from: https://www.jcgt.org/published/0009/03/02/
inline uint32_t xxhash32(const vec3u &p) {
    uint32_t h32 = p.z + JTX_XXHASH32_PRIME32_5 + p.x * JTX_XXHASH32_PRIME32_3;
    h32          = JTX_XXHASH32_PRIME32_4 * (h32 << 17 | h32 >> 32 - 17);
    h32 += p.y * JTX_XXHASH32_PRIME32_3;
    h32 = JTX_XXHASH32_PRIME32_4 * (h32 << 17 | h32 >> 32 - 17);
    h32 = JTX_XXHASH32_PRIME32_2 * (h32 ^ h32 >> 15);
    h32 = JTX_XXHASH32_PRIME32_3 * (h32 ^ h32 >> 13);
    return h32 ^ (h32 >> 16);
}

// RXS-M-XS: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
inline uint32_t pcg(const uint32_t seed) {
    const uint32_t state = seed * 747796405u + 2891336453u;
    const uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

}// namespace jtx