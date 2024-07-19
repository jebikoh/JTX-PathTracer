#pragma once

#include<cmath>
#include<stdexcept>
#include <jtxlib/math/constants.hpp>
#include <jtxlib/math/numerical.hpp>
#include <jtxlib/util/assert.hpp>

namespace jtx {
    JTX_NUM_ONLY_T
    class Vec2 {
    public:
        T x, y;

        // Check for NaN
        [[nodiscard]] inline bool valid() const {
            return !(jtx::isNaN(x) || jtx::isNaN(y));
        }

        //region Constructors
        Vec2() : x(JTX_ZERO), y(JTX_ZERO) {};

        Vec2(T x, T y) : x(x), y(y) { ASSERT(valid()); };

        Vec2(const Vec2 &other) : x(other.x), y(other.y) { ASSERT(valid()); };

        template<typename U>
        explicit Vec2(const Vec2<U> &other) : x(T(other.x)), y(T(other.y)) { ASSERT(valid()); };

        ~Vec2() = default;
        //endregion

        //region Unary operators
        inline Vec2 operator-() const {
            return {-x, -y};
        }

        inline Vec2 operator+() const {
            return {+x, +y};
        }

        inline Vec2 &operator++() {
            x++;
            y++;
            return *this;
        }

        inline Vec2 operator++(int) {
            Vec2 temp = *this;
            x++;
            y++;
            return temp;
        }

        inline Vec2 &operator--() {
            x--;
            y--;
            return *this;
        }

        inline Vec2 operator--(int) {
            Vec2 temp = *this;
            x--;
            y--;
            return temp;
        }
        //endregion

        //region Binary operators
        inline Vec2 &operator=(const Vec2 &other) {
            ASSERT(other.valid());
            x = other.x;
            y = other.y;
            return *this;
        }

        inline bool operator==(const Vec2 &other) const {
            return (x == other.x) && (y == other.y);
        }

        inline bool operator!=(const Vec2 &other) const {
            return (x != other.x) || (y != other.y);
        }

        inline Vec2 operator+(const Vec2 &other) const {
            return {x + other.x, y + other.y};
        }

        inline Vec2 operator+(const T scalar) const {
            return {x + scalar, y + scalar};
        }

        inline Vec2 operator-(const Vec2 &other) const {
            return {x - other.x, y - other.y};
        }

        inline Vec2 operator-(const T scalar) const {
            return {x - scalar, y - scalar};
        }

        inline Vec2 operator*(const Vec2 &other) const {
            return {x * other.x, y * other.y};
        }

        inline Vec2 operator*(T scalar) const {
            return {x * scalar, y * scalar};
        }

        friend inline Vec2 operator*(T scalar, const Vec2 &v) {
            return v * scalar;
        }

        inline Vec2 operator/(const Vec2 &other) const {
            ASSERT(JTX_ZERO != other.x && JTX_ZERO != other.y);
            return {x / other.x, y / other.y};
        }

        inline Vec2 operator/(T scalar) const {
            ASSERT(JTX_ZERO != scalar);
            return {x / scalar, y / scalar};
        }

        friend inline Vec2 operator/(T scalar, const Vec2 &v) {
            ASSERT(JTX_ZERO != scalar);
            return {scalar / v.x, scalar / v.y};
        }
        //endregion

        //region In-place Assignment Operators
        inline Vec2 &operator+=(const Vec2 &other) {
            x += other.x;
            y += other.y;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator+=(const T scalar) {
            x += scalar;
            y += scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator-=(const Vec2 &other) {
            x -= other.x;
            y -= other.y;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator-=(const T scalar) {
            x -= scalar;
            y -= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator*=(const Vec2 &other) {
            x *= other.x;
            y *= other.y;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator/=(const Vec2 &other) {
            x /= other.x;
            y /= other.y;
            ASSERT(valid());
            return *this;
        }

        inline Vec2 &operator/=(T scalar) {
            x /= scalar;
            y /= scalar;
            ASSERT(valid());
            return *this;
        }

        inline const T &operator[](int index) const {
            ASSERT(index >= 0 && index < 2);
            return (&x)[index];
        }

        inline T &operator[](int index) {
            ASSERT(index >= 0 && index < 2);
            return (&x)[index];
        }
        //endregion

        //region Member functions
        [[nodiscard]] inline T dot(const Vec2 &other) const {
            return x * other.x + y * other.y;
        }

        [[nodiscard]] inline T dot(const T _x, const T _y) const {
            return this->x * _x + this->y * _y;
        }

        inline Vec2 &abs() {
            x = std::abs(x);
            y = std::abs(y);
            return *this;
        }

        inline T absdot(const Vec2 &other) {
            return std::abs(dot(other));
        }

        [[nodiscard]] inline float len() const { return std::sqrt(x * x + y * y); }

        inline Vec2 &normalize() {
            float l = len();
            if (l != 0) {
                *this /= l;
            }
            return *this;
        }

        inline Vec2 &ceil() {
            x = jtx::ceil(x);
            y = jtx::ceil(y);
            return *this;
        }

        inline Vec2 &floor() {
            x = jtx::floor(x);
            y = jtx::floor(y);
            return *this;
        }

        inline auto min() const {
            return std::min(x, y);
        }

        inline auto max() const {
            return std::max(x, y);
        }

        inline T hprod() const {
            return x * y;
        }
        //endregion
    };

    typedef Vec2<int> Vec2i;
    typedef Vec2<float> Vec2f;

    JTX_NUM_ONLY_T
    using Point2 = Vec2<T>;

    typedef Point2<int> Point2i;
    typedef Point2<float> Point2f;

    inline std::string to_string(const Vec2f &vec) {
        return "Vec2f(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ")";
    }

    inline std::string to_string(const Vec2i &vec) {
        return "Vec2i(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ")";
    }
}// namespace jtx
