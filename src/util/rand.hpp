#pragma once

#include "../rt.hpp"

static constexpr uint32_t PCG32_MULT_32 = 747796405u;
static constexpr uint32_t PCG32_INCR_32 = 2891336453u;

static constexpr uint32_t PCG32_DEFAULT_STATE_32 = 0x853c49e5u;
static constexpr uint32_t PCG32_DEFAULT_OFFST_32 = 0xDA3E39CBu;

static constexpr float PCG32_FP_SCALE = 0x1p-32f;

// Modified version of RNG in PBRTv4
class RNG {
public:
    RNG() {
        state_ = 0;
        setSeed(PCG32_DEFAULT_STATE_32, PCG32_DEFAULT_OFFST_32);
    }

    explicit RNG(const uint32_t seed, const uint32_t offset = PCG32_DEFAULT_OFFST_32) {
        state_ = 0;
        setSeed(seed, offset);
    }

    void setSeed(const uint32_t seed, const uint32_t offset = PCG32_DEFAULT_OFFST_32) {
        state_ = 0;
        pcg();
        state_ += seed + offset;
        pcg();
    }

    float randomFloat() {
#ifdef USE_PCG_24_BIT_MASKING
        return (pcg() & 0xFFFFFF) / 16777216.0f;
#else
        return jtx::min(jtx::ONE_MINUS_EPSILON, pcg() * PCG32_FP_SCALE);
#endif
    }

private:
    uint32_t pcg() {
        const auto state = state_;
        state_           = state_ * PCG32_MULT_32 + PCG32_INCR_32;
        const auto word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 2u) ^ word;
    }

    uint32_t state_;
};


// Thread-managed version of the above PCG
uint32_t randPCG();

inline Float randomFloat() {
    return (randPCG() & 0xFFFFFF) / 16777216.0f;
}

inline Float randomFloat(const Float min, const Float max) {
    return min + (max - min) * randomFloat();
}

inline Vec3 randomVec3() {
    return {randomFloat(), randomFloat(), randomFloat()};
}

inline Vec3 randomVec3(Float min, Float max) {
    return {randomFloat(min, max), randomFloat(min, max), randomFloat(min, max)};
}

inline Vec3 randomUnitVector() {
    const float z = randomFloat() * 2.0f - 1.0f;
    const float a = randomFloat() * 2.0f * PI;
    const float r = jtx::sqrt(1.0f - z * z);
    return {r * jtx::cos(a), r * jtx::sin(a), z};
}

inline Vec3 randomOnHemisphere(const Vec3 &normal) {
    auto p = randomUnitVector();
    return jtx::dot(p, normal) > 0 ? p : -p;
}

inline Vec3 randomInUnitDisk() {
    while (true) {
        auto p = Vec3(randomFloat(-1, 1), randomFloat(-1, 1), 0);
        if (p.lenSqr() < 1) return p;
    }
}

inline bool nearZero(const Vec3 &v) {
    constexpr Float s = 1e-8;
    return (jtx::abs(v.x) < s) && (jtx::abs(v.y) < s) && (jtx::abs(v.z) < s);
}