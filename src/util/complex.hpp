#pragma once
#include "../rt.hpp"

struct Complex {
    float r;
    float i;

    HOSTDEV Complex()
        : r(0),
          i(0) {}
    HOSTDEV explicit Complex(const float r)
        : r(r),
          i(0){};
    HOSTDEV Complex(const float r, const float i)
        : r(r),
          i(i) {}

    HOSTDEV Complex operator-() const { return {-r, -i}; }

    HOSTDEV Complex operator+(const Complex &c) const { return {r + c.r, i + c.i}; }
    HOSTDEV Complex operator+(const float f) const { return {r + f, i}; }

    HOSTDEV Complex operator-(const Complex &c) const { return {r - c.r, i - c.i}; }
    HOSTDEV Complex operator-(const float f) const { return {r - f, i}; }

    HOSTDEV Complex operator*(const Complex &c) const {
        return {r * c.r - i * c.i, r * c.i + i * c.r};
    }
    HOSTDEV Complex operator*(const float f) const { return *this * Complex(f); }
    HOSTDEV Complex operator/(const Complex &c) const {
        const float scale = 1 / (c.r * c.r + c.i * c.i);
        return {(r * c.r + i * c.i) * scale, (i * c.r - r * c.i) * scale};
    }

    friend HOSTDEV Complex operator+(const float f, const Complex &c) { return {f + c.r, c.i}; }
    friend HOSTDEV Complex operator-(const float f, const Complex &c) { return {f - c.r, -c.i}; }
    friend HOSTDEV Complex operator*(const float f, const Complex &c) { return Complex(f) * c; }
    friend HOSTDEV Complex operator/(const float f, const Complex &c) { return Complex(f) / c; }
};

HOSTDEV inline float norm(const Complex &c) {
    return c.r * c.r + c.i * c.i;
}

HOSTDEV inline float abs(const Complex &c) {
    return jtx::sqrt(norm(c));
}

HOSTDEV inline Complex sqrt(const Complex &c) {
    const float n = abs(c);
    const float t1 = sqrt(0.5f * (n + jtx::abs(c.r)));
    const float t2 = 0.5f * c.i / t1;

    if (n == 0) return {};
    if (c.r >= 0) return {t1, t2};
    return {jtx::abs(t2), jtx::copysign(t1, c.i)};
}