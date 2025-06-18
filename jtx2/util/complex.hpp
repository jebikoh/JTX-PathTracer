#pragma once

#include <jtx.hpp>

namespace jtx {
struct Complex {
    float r;
    float i;

    Complex() : r(0.0f), i(0.0f) {}
    explicit Complex(const float r) : r(r), i(0.0f) {}
    Complex(const float r, const float i) : r(r), i(i) {}

    Complex operator-() const { return {-r, -i}; }

    Complex operator+(const Complex &c) const { return {r + c.r, i + c.i}; }
    Complex operator+(const float f) const { return {r + f, i}; }

    Complex operator-(const Complex &c) const { return {r - c.r, i - c.i}; }
    Complex operator-(const float f) const { return {r - f, i}; }

    Complex operator*(const Complex &c) const {
        return {r * c.r - i * c.i, r * c.i + i * c.r};
    }
    Complex operator*(const float f) const { return *this * Complex(f, 0.0f); }

    Complex operator/(const Complex &c) const {
        const float scale = 1 / (c.r * c.r + c.i * c.i);
        return {(r * c.r + i * c.i) * scale, (i * c.r - r * c.i) * scale};
    }
    Complex operator/(const float f) const { return *this / Complex(f, 0.0f); }

    friend Complex operator+(const float f, const Complex &c) { return {f + c.r, c.i}; }
    friend Complex operator-(const float f, const Complex &c) { return {f - c.r, -c.i}; }
    friend Complex operator*(const float f, const Complex &c) { return Complex(f) * c; }
    friend Complex operator/(const float f, const Complex &c) { return Complex(f) / c; }
};

inline float Norm(const Complex &c) {
    return c.r * c.r + c.i * c.i;
}

inline float Abs(const Complex &c) {
    return Sqrt(Norm(c));
}

inline Complex Sqrt(const Complex &c) {
    const float n = Abs(c);
    if (n == 0) return {};

    const float t1 = Sqrt(0.5f * (n + Abs(c.r)));
    const float t2 = 0.5f * c.i / t1;

    if (c.r >= 0) return {t1, t2};
    return {Abs(t2), CopySign(t1, c.i)};
}

}

