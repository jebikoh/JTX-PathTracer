#pragma once

#include<cmath>
#include<stdexcept>
#include <jtxlib/math/constants.hpp>
#include <jtxlib/math/numerical.hpp>
#include <jtxlib/util/assert.hpp>
#include <jtxlib/math/math.hpp>

namespace jtx {
    JTX_NUM_ONLY_T
    class Vec3 {
    public:
        T x, y, z;

        // Check for NaN
        [[nodiscard]] inline bool valid() const {
            return !(jtx::isNaN(x) || jtx::isNaN(y) || jtx::isNaN(z));
        }

        //region Constructors
        Vec3() : x(JTX_ZERO), y(JTX_ZERO), z(JTX_ZERO) {};

        Vec3(T x, T y, T z) : x(x), y(y), z(z) { ASSERT(valid()); };

        Vec3(const Vec3 &other) : x(other.x), y(other.y), z(other.z) { ASSERT(valid()); };

        template<typename U>
        explicit Vec3(const Vec3<U> &other) : x(T(other.x)), y(T(other.y)), z(T(other.z)) { ASSERT(valid()); };

        ~Vec3() = default;
        //endregion

        //region Unary operators
        inline Vec3 operator-() const {
            return {-x, -y, -z};
        }

        inline Vec3 operator+() const {
            return {+x, +y, +z};
        }

        inline Vec3 &operator++() {
            x++;
            y++;
            z++;
            return *this;
        }

        inline Vec3 operator++(int) {
            Vec3 temp = *this;
            x++;
            y++;
            z++;
            return temp;
        }

        inline Vec3 &operator--() {
            x--;
            y--;
            z--;
            return *this;
        }

        inline Vec3 operator--(int) {
            Vec3 temp = *this;
            x--;
            y--;
            z--;
            return temp;
        }
        //endregion

        //region Binary operators
        inline Vec3 &operator=(const Vec3 &other) {
            ASSERT(other.valid());
            x = other.x;
            y = other.y;
            z = other.z;
            return *this;
        }

        inline bool operator==(const Vec3 &other) const {
            return (x == other.x) && (y == other.y) && (z == other.z);
        }

        inline bool operator!=(const Vec3 &other) const {
            return (x != other.x) || (y != other.y) || (z != other.z);
        }

        inline Vec3 operator+(const Vec3 &other) const {
            return {x + other.x, y + other.y, z + other.z};
        }

        inline Vec3 operator+(const T scalar) const {
            return {x + scalar, y + scalar, z + scalar};
        }

        inline Vec3 operator-(const Vec3 &other) const {
            return {x - other.x, y - other.y, z - other.z};
        }

        inline Vec3 operator-(const T scalar) const {
            return {x - scalar, y - scalar, z - scalar};
        }

        inline Vec3 operator*(const Vec3 &other) const {
            return {x * other.x, y * other.y, z * other.z};
        }

        inline Vec3 operator*(T scalar) const {
            return {x * scalar, y * scalar, z * scalar};
        }

        friend inline Vec3 operator*(T scalar, const Vec3 &v) {
            return v * scalar;
        }

        inline Vec3 operator/(const Vec3 &other) const {
            ASSERT(JTX_ZERO != other.x && JTX_ZERO != other.y && JTX_ZERO != other.z);
            return {x / other.x, y / other.y, z / other.z};
        }

        inline Vec3 operator/(T scalar) const {
            ASSERT(JTX_ZERO != scalar);
            return {x / scalar, y / scalar, z / scalar};
        }

        friend inline Vec3 operator/(T scalar, const Vec3 &v) {
            ASSERT(JTX_ZERO != scalar);
            return {scalar / v.x, scalar / v.y, scalar / v.z};
        }
        //endregion

        //region In-place Assignment Operators
        inline Vec3 &operator+=(const Vec3 &other) {
            x += other.x;
            y += other.y;
            z += other.z;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator+=(const T scalar) {
            x += scalar;
            y += scalar;
            z += scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator-=(const Vec3 &other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator-=(const T scalar) {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator*=(const Vec3 &other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator/=(const Vec3 &other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            ASSERT(valid());
            return *this;
        }

        inline Vec3 &operator/=(T scalar) {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            ASSERT(valid());
            return *this;
        }

        inline const T &operator[](int index) const {
            ASSERT(index >= 0 && index < 3);
            return (&x)[index];
        }

        inline T &operator[](int index) {
            ASSERT(index >= 0 && index < 3);
            return (&x)[index];
        }
        //endregion

        //region Member functions
        [[nodiscard]] inline T dot(const Vec3 &other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        [[nodiscard]] inline T dot(const T _x, const T _y, const T _z) const {
            return this->x * _x + this->y * _y + this->z * _z;
        }

        [[nodiscard]] inline Vec3 cross(const Vec3 &other) const {
            return {jtx::dop(y, other.z, z, other.y),
                    jtx::dop(z, other.x, x, other.z),
                    jtx::dop(x, other.y, y, other.x)};
        }


        inline Vec3 &abs() {
            x = std::abs(x);
            y = std::abs(y);
            z = std::abs(z);
            return *this;
        }

        inline T absdot(const Vec3 &other) {
            return std::abs(dot(other));
        }

        [[nodiscard]] inline float lenSqr() const {
            return x * x + y * y + z * z;
        }

        [[nodiscard]] inline float len() const { return std::sqrt(lenSqr()); }

        inline Vec3 &normalize() {
            float l = len();
            if (l != 0) {
                (*this) /= l;
            }
            return *this;
        }

        [[nodiscard]] inline T l1norm() const {
            return std::abs(x) + std::abs(y) + std::abs(z);
        }

        inline Vec3 &ceil() {
            x = jtx::ceil(x);
            y = jtx::ceil(y);
            z = jtx::ceil(z);
            return *this;
        }

        inline Vec3 &floor() {
            x = jtx::floor(x);
            y = jtx::floor(y);
            z = jtx::floor(z);
            return *this;
        }

        inline auto min() const {
            return std::min(z, std::min(x, y));
        }

        inline auto max() const {
            return std::max(z, std::max(x, y));
        }

        [[nodiscard]] inline T hprod() const {
            return x * y * z;
        }

        inline Vec3 &align(const Vec3 &other) {
            if ((*this).dot(other) < 0.0f) {
                (*this) = -(*this);
            }
            return *this;
        }
        //endregion
    };

    //region Type aliases
    typedef Vec3<int> Vec3i;
    typedef Vec3<float> Vec3f;
    typedef Vec3<double> Vec3d;

    JTX_NUM_ONLY_T
    using Point3 = Vec3<T>;
    typedef Point3<int> Point3i;
    typedef Point3<float> Point3f;

    typedef Vec3<float> Normal3f;
    //endregion

    inline std::string to_string(const Vec3f &vec) {
        return "Vec3f(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
    }

    inline std::string to_string(const Vec3i &vec) {
        return "Vec3i(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
    }
}// namespace jtx
