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
    HOSTDEV Complex operator-(const Complex &c) const { return {r - c.r, i - c.i}; }
    friend HOSTDEV Complex operator+(const float f, const Complex &c) { return {f + c.r, c.i}; }
    friend HOSTDEV Complex operator-(const float f, const Complex &c) { return {f - c.r, -c.i}; }

    HOSTDEV Complex operator*(const Complex &c) const {
        return {r * c.r - i * c.i, r * c.i + i * c.r};
    }
    HOSTDEV Complex operator/(const Complex &c) const {
        const float scale = 1 / c.r * c.r + c.i * c.i;
        return {(r * c.r + i * c.i) * scale, (i * c.r - r * c.i) * scale};
    }
    friend HOSTDEV Complex operator*(const float f, const Complex &c) { return Complex(f) * c; }
    friend HOSTDEV Complex operator/(const float f, const Complex &c) { return Complex(f) / c; }
};