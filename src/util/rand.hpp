#pragma once

#include "../rt.hpp"
#include "hash.hpp"

static constexpr auto RXS_M_XS_MULT = 747796405u;
static constexpr auto RXS_M_XS_INCR = 2891336453u;

static constexpr auto FNV_1_PRIME = 16777619u;
static constexpr auto FNV_1_OFFST = 2166136261u;

// FNV-1a for 3 32-bit integer seeding hash
inline uint32_t fnv1a_3(const uint32_t x, const uint32_t y, const uint32_t n) {
    uint32_t hash = FNV_1_OFFST;

    hash ^= x;
    hash *= FNV_1_PRIME;
    hash ^= y;
    hash *= FNV_1_PRIME;
    hash ^= n;
    hash *= FNV_1_PRIME;

    return hash;
}

inline uint32_t fnv1a_4(const uint32_t x, uint32_t y, uint32_t n, uint32_t stratum) {
    uint32_t hash = FNV_1_OFFST;

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
    RNG()
        : state_(0) {}

    explicit RNG(const uint32_t seed) {
        state_ = 0;
        init(seed);
    }

    RNG(const uint32_t x, const uint32_t y, const uint32_t n) {
        const auto seed = fnv1a_3(x, y, n);
        init(seed);
    }

    void setSeed(const uint32_t seed) {
        this->state_ = seed;
    }

    // Sample single values between 0 and 1
    template<typename T>
    T sample();

    // Sampling within range
    template<typename T>
    T sample(const T min, const T max);

    // Lemire's Method
    // https://www.pcg-random.org/posts/bounded-rands.html
    // https://lemire.me/blog/2019/06/06/nearly-divisionless-random-integer-generation-on-various-systems/
    // https://arxiv.org/abs/1805.10941
    uint32_t sampleRange(const int range) {
        uint32_t t = (-range) % range;

        uint64_t m;
        uint32_t l;
        do {
            uint32_t x = advance();
            m          = uint64_t(x) * uint64_t(range);
            l          = uint32_t(m);
        } while (l < t);
        return m >> 32;
    }

    Vec3 sampleUnitVector();

    Vec3 sampleOnHemisphere(const Vec3 &normal);

    Vec3 sampleUnitDisc();

private:
    void init(const uint32_t seed) {
        advance();
        state_ += seed;
        advance();
    }

    uint32_t advance() {
        const auto state = state_;
        state_           = state_ * RXS_M_XS_MULT + RXS_M_XS_INCR;
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

template<typename T>
inline T sample() {
    T::unimplemented;
}

template<>
inline uint32_t RNG::sample<uint32_t>() {
    return advance();
}

template<>
inline float RNG::sample<float>() {
    return (advance() & 0xFFFFFF) / 16777216.0f;
}

template<>
inline Vec2f RNG::sample<Vec2f>() {
    return {sample<float>(), sample<float>()};
}

template<>
inline Vec3 RNG::sample<Vec3>() {
    return {sample<float>(), sample<float>(), sample<float>()};
}

// Sampling within range
template<typename T>
inline T RNG::sample(const T min, const T max) {
    T::unimplemented;
}

template<>
inline float RNG::sample<float>(const float min, const float max) {
    return min + (max - min) * sample<float>();
}

template<>
inline Vec2f RNG::sample<Vec2f>(const Vec2f min, const Vec2f max) {
    return min + (max - min) * sample<Vec2f>();
}

template<>
inline Vec3 RNG::sample<Vec3>(const Vec3 min, const Vec3 max) {
    return min + (max - min) * sample<Vec3>();
}

inline Vec3 RNG::sampleUnitVector() {
    const float z = sample<float>() * 2.0f - 1.0f;
    const float a = sample<float>() * 2.0f * PI;
    const float r = jtx::sqrt(1.0f - z * z);
    return {r * jtx::cos(a), r * jtx::sin(a), z};
}

inline Vec3 RNG::sampleOnHemisphere(const Vec3 &normal) {
    auto p = sampleUnitVector();
    return jtx::dot(p, normal) > 0 ? p : -p;
}

inline Vec3 RNG::sampleUnitDisc() {
    while (true) {
        auto p = Vec3(sample<float>(-1, 1), sample<float>(-1, 1), 0);
        if (p.lenSqr() < 1) return p;
    }
}