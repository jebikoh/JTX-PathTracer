#pragma once

constexpr float PI_OVER_4 = PI / 4;
constexpr float PI_OVER_2 = PI / 2;
constexpr float INV_TWO_PI = 1.0f / (2.0f * PI);
constexpr float TWO_PI = 2.0F * PI;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_4_PI = 1.0f / (4.0f * PI);

inline Vec3 sampleUniformSphere(Vec2f u) {
    // $z = 1 - 2\xi_1$
    const float z = 1 - 2 * u[1];
    // $\sqrt{1 - z^2}$
    const float a = jtx::safeSqrt(1 - z * z);
    // $2\pi\xi_2$
    const float phi = TWO_PI * u[1];
    return {jtx::cos(phi) * a, jtx::sin(phi) * a, z};
}

inline float uniformSpherePDF() { return INV_4_PI; }

inline Vec2f sampleUniformDiskPolar(const Vec2f &u) {
    const float r = jtx::sqrt(u.x);
    const float theta = 2 * PI * u.y;
    return {r * jtx::cos(theta), r * jtx::sin(theta)};
}

inline Vec2f sampleUniformDiskConcentric(const Vec2f &u) {
    const auto uOffset = 2.0f * u - Vec2f(1.0f, 1.0f);
    if (uOffset.x == 0 && uOffset.y == 0) {
        return {0.0f, 0.0f};
    }

    float r, theta;
    if (jtx::abs(uOffset.x) > jtx::abs(uOffset.y)) {
        // X is dominant axis
        r = uOffset.x;
        theta = PI_OVER_4 * (uOffset.y / uOffset.x);
    } else {
        // Y is dominant axis
        r = uOffset.y;
        theta = PI_OVER_2 - PI_OVER_4 * (uOffset.x / uOffset.y);
    }

    return {r * jtx::cos(theta), r * jtx::sin(theta)};
}

inline Vec3 sampleUniformHemisphere(Vec2f u) {
    const float sinTheta = jtx::safeSqrt(1 - u.x * u.x);
    const float phi = 2 * PI * u.y;
    return {jtx::cos(phi) * sinTheta, jtx::sin(phi) * sinTheta, u.x};
}

inline float uniformHemispherePDF() { return INV_TWO_PI; }

inline Vec3 sampleCosineHemisphere(const Vec2f &u) {
    auto diskU = sampleUniformDiskConcentric(u);
    return {diskU.x, diskU.y, jtx::safeSqrt(1 - diskU.x * diskU.x - diskU.y * diskU.y)};
}

inline float cosineHemispherePDF(const float cosTheta) {
    return cosTheta * INV_PI;
}