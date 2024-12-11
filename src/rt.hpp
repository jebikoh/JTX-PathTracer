#pragma once

#include <jtxlib/math.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <random>

#define RT_DOUBLE_PRECISION

#ifdef RT_DOUBLE_PRECISION
using Float = double;
using Vec3  = jtx::Vec3d;
using Ray   = jtx::Rayd;
constexpr Float INF = jtx::INFINITY_D;
constexpr Float PI  = jtx::PI;
#else
using Float = float;
using Vec3  = jtx::Vec3f;
using Ray   = jtx::Rayf;
constexpr Float INF = jtx::INFINITY_F;
constexpr Float PI  = jtx::PI_F;
#endif

inline Float radians(const Float degrees) {
    return degrees * PI / static_cast<float>(180.0);
}

#include "color.hpp"

inline Float randomFloat() {
    static std::uniform_real_distribution<Float> dist(0, 1);
    static std::mt19937 gen;
    return dist(gen);
}

inline Float randomFloat(Float min, Float max) {
    return min + (max - min) * randomFloat();
}

inline Vec3 randomVec3() {
    return {randomFloat(), randomFloat(), randomFloat()};
}

inline Vec3 randomVec3(Float min, Float max) {
    return {randomFloat(min, max), randomFloat(min, max), randomFloat(min, max)};
}

inline Vec3 randomUnitVector() {
    while (true) {
        auto p = randomVec3(-1, 1);
        auto lensq = p.lenSqr();
        if (1e-160 < lensq && lensq <= 1) return p / jtx::sqrt(lensq);
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