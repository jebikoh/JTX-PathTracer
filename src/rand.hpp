#pragma once

#include "rt.hpp"

inline Float radians(const Float degrees) {
    return degrees * PI / static_cast<float>(180.0);
}

uint32_t randPCG();

// TODO: remove support for double precision
inline Float randomFloat() {
    return (randPCG() & 0xFFFFFF) / 16777216.0f;
    // static std::uniform_real_distribution<Float> dist(0, 1);
    // static std::mt19937 gen;
    // return dist(gen);
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
    // const float z = randomFloat() * 2.0f - 1.0f;
    // const float a = randomFloat() * 2.0f * PI;
    // const float r = jtx::sqrt(1.0f - z * z);
    // return {r * jtx::cos(a), r * jtx::sin(a), z};

    while (true) {
        auto p           = randomVec3(-1, 1);
        if (const auto lensq = p.lenSqr(); 1e-160 < lensq && lensq <= 1) return p / jtx::sqrt(lensq);
    }
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