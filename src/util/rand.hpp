#pragma once

#include "../rt.hpp"
#include "hash.hpp"

static constexpr auto PCG32_DEFAULT_STATE  = 0x853c49e6748fea9bULL;
static constexpr auto PCG32_DEFAULT_STREAM = 0xda3e39cb94b95bdbULL;
static constexpr auto PCG32_MULT           = 0x5851f42d4c957f2dULL;

static constexpr auto RXS_M_XS_MULT = 747796405u;
static constexpr auto RXS_M_XS_INCR = 2891336453u;

static constexpr auto FNV_1_PRIME = 16777619u;
static constexpr auto FNV_1_OFFST = 2166136261u;

class PCG32 {
public:
    PCG32()
        : state_(PCG32_DEFAULT_STATE),
          inc_(PCG32_DEFAULT_STREAM) {}

    PCG32(const uint64_t index, const uint64_t offset) {
        setSequence(index, offset);
    }

    explicit PCG32(const uint64_t index) {
        setSequence(index);
    }
    void setSequence(const uint64_t index, const uint64_t offset) {
        state_ = 0u;
        inc_   = (index << 1u) | 1u;
        sampleU32();
        state_ += offset;
        sampleU32();
    }

    void setSequence(const uint64_t index) {
        setSequence(index, mixBits(index));
    }

    void advance(const int64_t idelta) {
        uint64_t curMult = PCG32_MULT, curPlus = inc_, accMult = 1u;
        uint64_t accPlus = 0u, delta = static_cast<uint64_t>(idelta);
        while (delta > 0) {
            if (delta & 1) {
                accMult *= curMult;
                accPlus = accPlus * curMult + curPlus;
            }
            curPlus = (curMult + 1) * curPlus;
            curMult *= curMult;
            delta /= 2;
        }
        state_ = accMult * state_ + accPlus;
    }

    uint32_t sampleU32() {
        const uint64_t oldState   = state_;
        state_                    = oldState * PCG32_MULT + inc_;
        const uint32_t xorShifted = static_cast<uint32_t>(((oldState >> 18u) ^ oldState) >> 27u);
        const uint32_t rot        = static_cast<uint32_t>(oldState >> 59u);
        return (xorShifted >> rot) | (xorShifted << ((~rot + 1u) & 31));
    }

    float sampleFP() {
        return jtx::min(jtx::ONE_MINUS_EPSILON, sampleU32() * 0x1p-32f);
    }

private:
    uint64_t state_;
    uint64_t inc_;
};

//RXS-M-XS
class RNG {

};

// Used exclusively for quick single floats or small hashes
// RXS-M-XS for 32-bit hashing
inline uint32_t pcgHash(const uint32_t x) {
    const uint32_t state = x * RXS_M_XS_MULT + RXS_M_XS_INCR;
    const uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// FNV-1a for 3 32-bit integer seeding hash
inline uint32_t fnv1a_3(uint32_t x, uint32_t y, uint32_t s) {
    uint32_t hash  = FNV_1_OFFST;
    uint32_t prime = FNV_1_PRIME;

    hash ^= x;
    hash *= prime;
    hash ^= y;
    hash *= prime;
    hash ^= s;
    hash *= prime;

    return hash;
}

// Thread local PCG generator
uint32_t pcgRand();

inline Float randomFloat() {
    return (pcgRand() & 0xFFFFFF) / 16777216.0f;
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