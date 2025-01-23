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

// FNV-1a for 3 32-bit integer seeding hash
inline uint32_t fnv1a_3(const uint32_t x, const uint32_t y, const uint32_t n) {
    uint32_t hash  = FNV_1_OFFST;

    hash ^= x;
    hash *= FNV_1_PRIME;
    hash ^= y;
    hash *= FNV_1_PRIME;
    hash ^= n;
    hash *= FNV_1_PRIME;

    return hash;
}

inline uint32_t fnv1a_4(const uint32_t x, uint32_t y, uint32_t n, uint32_t stratum) {
    uint32_t hash  = FNV_1_OFFST;

    hash ^= x;
    hash *= FNV_1_PRIME;
    hash ^= y;
    hash *= FNV_1_PRIME;
    hash ^= n;
    hash *= FNV_1_PRIME;
    hash ^= stratum;
    hash *= FNV_1_PRIME;

    return hash;
}

// RXS-M-XS
class RNG {
public:
    RNG() : state_(0) {}

    explicit RNG(const uint32_t seed) : state_(seed) {}

    RNG(const uint32_t x, const uint32_t y, const uint32_t n) {
        state_ = fnv1a_3(x, y, n);
    }

    void setSeed(const uint32_t seed) {
        this->state_ = seed;
    }

    float sampleFP() {
        return (advance() & 0xFFFFFF) / 16777216.0f;
    }

    float sampleFP(const float min, const float max) {
        return min + (max - min) * sampleFP();
    }

    Vec2f sampleVec2() {
        return {sampleFP(), sampleFP()};
    }

    Vec3 sampleVec3() {
        return {sampleFP(), sampleFP(), sampleFP()};
    }

    Vec3 sampleVec3(const float min, const float max) {
        return {sampleFP(min, max),sampleFP(min, max), sampleFP(min, max)};
    }

    Vec3 sampleUnitVector() {
        const float z = sampleFP() * 2.0f - 1.0f;
        const float a = sampleFP() * 2.0f * PI;
        const float r = jtx::sqrt(1.0f - z * z);
        return {r * jtx::cos(a), r * jtx::sin(a), z};
    }

    Vec3 sampleOnHemisphere(const Vec3 &normal) {
        auto p = sampleUnitVector();
        return jtx::dot(p, normal) > 0 ? p : -p;
    }

    Vec3 sampleUnitDisc() {
        while (true) {
            auto p = Vec3(sampleFP(-1, 1), sampleFP(-1, 1), 0);
            if (p.lenSqr() < 1) return p;
        }
    }

private:
    uint32_t advance() {
        const auto state = state_;
        state_ = state_ * RXS_M_XS_MULT + RXS_M_XS_INCR;
        const auto word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 2u) ^ word;
    }

    uint32_t state_;
};

// RXS-M-XS for 32-bit hashing
inline uint32_t pcgHash(const uint32_t x) {
    const uint32_t state = x * RXS_M_XS_MULT + RXS_M_XS_INCR;
    const uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

inline bool nearZero(const Vec3 &v) {
    constexpr Float s = 1e-8;
    return (jtx::abs(v.x) < s) && (jtx::abs(v.y) < s) && (jtx::abs(v.z) < s);
}