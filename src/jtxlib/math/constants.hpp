#pragma once

#include <limits>

namespace jtx {
    // TODO: expand as needed
    // Structure from https://github.com/behindthepixels/EDXUtil/
    static struct Zero {
        inline explicit operator int() const { return 0; }

        inline explicit operator float() const { return 0.0f; }

        inline explicit operator double() const { return 0.0; }

        template<typename T>
        inline bool operator==(const T &other) const {
            return static_cast<T>(*this) == other;
        }

        template<typename T>
        inline bool operator!=(const T &other) const {
            return static_cast<T>(*this) != other;
        }
    } JTX_ZERO;

    constexpr double PI = 3.14159265358979323846;
    constexpr float PI_F = 3.14159265358979323846f;
    constexpr float BITS_16 = 65535.0f;

    constexpr float INFINITY_F = std::numeric_limits<float>::infinity();
    constexpr double INFINITY_D = std::numeric_limits<double>::infinity();

    constexpr float NEG_INFINITY_F = -std::numeric_limits<float>::infinity();
    constexpr double NEG_INFINITY_D = -std::numeric_limits<double>::infinity();
}

