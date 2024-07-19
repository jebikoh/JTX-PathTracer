#pragma once

#include <cmath>
#include <stdexcept>
#include <jtxlib/math/constants.hpp>
#include <jtxlib/math/numerical.hpp>
#include <jtxlib/util/assert.hpp>
#include <jtxlib/math/math.hpp>

namespace jtx {
    JTX_NUM_ONLY_T
    class Vec4 {
    public:
        T x, y, z, w;

        // Check for NaN
        [[nodiscard]] inline bool valid() const {
            return !(jtx::isNaN(x) || jtx::isNaN(y) || jtx::isNaN(z) || jtx::isNaN(w));
        }

        //region Constructors
        Vec4() : x(JTX_ZERO), y(JTX_ZERO), z(JTX_ZERO), w(JTX_ZERO) {};

        Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { ASSERT(valid()); };

        Vec4(const Vec4 &other) : x(other.x), y(other.y), z(other.z), w(other.w) { ASSERT(valid()); };

        template<typename U>
        explicit Vec4(const Vec4<U> &other) : x(T(other.x)), y(T(other.y)), z(T(other.z)), w(T(other.w)) {
            ASSERT(valid());
        };

        ~Vec4() = default;
        //endregion

        //region Unary operators
        inline Vec4 operator-() const {
            return {-x, -y, -z, -w};
        }

        inline Vec4 operator+() const {
            return {+x, +y, +z, +w};
        }

        inline Vec4 &operator++() {
            x++;
            y++;
            z++;
            w++;
            return *this;
        }

        inline Vec4 operator++(int) {
            Vec4 temp = *this;
            x++;
            y++;
            z++;
            w++;
            return temp;
        }

        inline Vec4 &operator--() {
            x--;
            y--;
            z--;
            w--;
            return *this;
        }

        inline Vec4 operator--(int) {
            Vec4 temp = *this;
            x--;
            y--;
            z--;
            w--;
            return temp;
        }
        //endregion

        //region Binary operators
        inline Vec4 &operator=(const Vec4 &other) {
            ASSERT(other.valid());
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
            return *this;
        }

        inline bool operator==(const Vec4 &other) const {
            return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
        }

        inline bool operator!=(const Vec4 &other) const {
            return (x != other.x) || (y != other.y) || (z != other.z) || (w != other.w);
        }

        inline Vec4 operator+(const Vec4 &other) const {
            return {x + other.x, y + other.y, z + other.z, w + other.w};
        }

        inline Vec4 operator+(const T scalar) const {
            return {x + scalar, y + scalar, z + scalar, w + scalar};
        }

        inline Vec4 operator-(const Vec4 &other) const {
            return {x - other.x, y - other.y, z - other.z, w - other.w};
        }

        inline Vec4 operator-(const T scalar) const {
            return {x - scalar, y - scalar, z - scalar, w - scalar};
        }

        inline Vec4 operator*(const Vec4 &other) const {
            return {x * other.x, y * other.y, z * other.z, w * other.w};
        }

        inline Vec4 operator*(T scalar) const {
            return {x * scalar, y * scalar, z * scalar, w * scalar};
        }

        friend inline Vec4 operator*(T scalar, const Vec4 &v) {
            return v * scalar;
        }

        inline Vec4 operator/(const Vec4 &other) const {
            ASSERT(JTX_ZERO != other.x && JTX_ZERO != other.y && JTX_ZERO != other.z && JTX_ZERO != other.w);
            return {x / other.x, y / other.y, z / other.z, w / other.w};
        }

        inline Vec4 operator/(T scalar) const {
            ASSERT(JTX_ZERO != scalar);
            return {x / scalar, y / scalar, z / scalar, w / scalar};
        }

        friend inline Vec4 operator/(T scalar, const Vec4 &v) {
            ASSERT(JTX_ZERO != scalar);
            return {scalar / v.x, scalar / v.y, scalar / v.z, scalar / v.w};
        }

        inline const T &operator[](int index) const {
            ASSERT(index >= 0 && index < 4);
            return (&x)[index];
        }

        inline T &operator[](int index) {
            ASSERT(index >= 0 && index < 4);
            return (&x)[index];
        }
        //endregion

        //region In-place Assignment Operators
        inline Vec4 &operator+=(const Vec4 &other) {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator+=(const T scalar) {
            x += scalar;
            y += scalar;
            z += scalar;
            w += scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator-=(const Vec4 &other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator-=(const T scalar) {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            w -= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator*=(const Vec4 &other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            w *= other.w;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            w *= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator/=(const Vec4 &other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            w /= other.w;
            ASSERT(valid());
            return *this;
        }

        inline Vec4 &operator/=(T scalar) {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
            ASSERT(valid());
            return *this;
        }
        //endregion

        //region Member functions
        [[nodiscard]] inline T dot(const Vec4 &other) const {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        [[nodiscard]] inline T dot(const T _x, const T _y, const T _z, const T _w) const {
            return this->x * _x + this->y * _y + this->z * _z + this->w * _w;
        }

        inline Vec4 &abs() {
            x = std::abs(x);
            y = std::abs(y);
            z = std::abs(z);
            w = std::abs(w);
            return *this;
        }

        inline T absdot(const Vec4 &other) {
            return std::abs(dot(other));
        }

        [[nodiscard]] inline float len() const { return std::sqrt(x * x + y * y + z * z + w * w); }

        inline Vec4 &normalize() {
            float l = len();
            if (l != 0) {
                (*this) /= l;
            }
            return *this;
        }

        inline Vec4 &ceil() {
            x = jtx::ceil(x);
            y = jtx::ceil(y);
            z = jtx::ceil(z);
            w = jtx::ceil(w);
            return *this;
        }

        inline Vec4 &floor() {
            x = jtx::floor(x);
            y = jtx::floor(y);
            z = jtx::floor(z);
            w = jtx::floor(w);
            return *this;
        }

        inline auto min() const {
            return std::min(std::min(z, w), std::min(x, y));
        }

        inline auto max() const {
            return std::max(std::max(z, w), std::max(x, y));
        }

        inline T hprod() const {
            return x * y * z * w;
        }
        //endregion
    };

    typedef Vec4<int> Vec4i;
    typedef Vec4<float> Vec4f;

    JTX_NUM_ONLY_T
    using Point4 = Vec4<T>;

    typedef Point4<int> Point4i;
    typedef Point4<float> Point4f;

    inline std::string to_string(const Vec4f &vec) {
        return "Vec4f(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ", " + std::to_string(vec.w) + ")";
    }

    inline std::string to_string(const Vec4i &vec) {
        return "Vec4i(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ", " + std::to_string(vec.w) + ")";
    }
}// namespace jtx
