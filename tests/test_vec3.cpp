#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/vecmath.hpp>
#include "./tconstants.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

//region Vec3: Constructors
TEST_CASE("Vec3f default constructor", "[Vec3]") {
    jtx::Vec3f v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vec3f constructor", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vec3f copy constructor", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(v1);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f template copy constructor", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3i v2(v1);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
}
//endregion

//region Vec3: Numerical validity
TEST_CASE("Vec3f valid", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.valid());
}

#ifdef JTX_TEST

TEST_CASE("Vec3f invalid", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN());
    REQUIRE(!v.valid());
}

#endif
//endregion

//region Vec3: Unary oeprators
TEST_CASE("Vec3f operator- on positive values", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = -v1;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == -2.0f);
    REQUIRE(v2.z == -3.0f);
}

TEST_CASE("Vec3f operator- on negative values", "[Vec3]") {
    jtx::Vec3f v1(-1.0f, -2.0f, -3.0f);
    jtx::Vec3f v2 = -v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f operator+", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = +v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f ++ postfix operator", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = v++;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f ++ prefix operator", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = ++v;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
    REQUIRE(v2.z == 4.0f);
}

TEST_CASE("Vec3f -- postfix operator", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = v--;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v.z == 2.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f -- prefix operator", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = --v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v.z == 2.0f);
    REQUIRE(v2.x == 0.0f);
    REQUIRE(v2.y == 1.0f);
    REQUIRE(v2.z == 2.0f);
}
//endregion

//region Vec3: Binary operators
TEST_CASE("Vec3f = operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    v2 = v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f == operator (equal)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(1.0f, 2.0f, 3.0f);
    REQUIRE(v1 == v2);
}

TEST_CASE("Vec3f == operator (unequal)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(1.0f, 2.0f, 4.0f);
    REQUIRE(!(v1 == v2));
}

TEST_CASE("Vec3f != operator (equal)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(1.0f, 2.0f, 3.0f);
    REQUIRE(!(v1 != v2));
}

TEST_CASE("Vec3f != operator (unequal)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(1.0f, 2.0f, 4.0f);
    REQUIRE(v1 != v2);
}

TEST_CASE("Vec3f + operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = v1 + v2;
    REQUIRE(v3.x == 5.0f);
    REQUIRE(v3.y == 7.0f);
    REQUIRE(v3.z == 9.0f);
}

TEST_CASE("Vec3f + operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    jtx::Vec3f v2 = v1 + s;
    REQUIRE(v2.x == 3.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 5.0f);
}

TEST_CASE("Vec3f - operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = v1 - v2;
    REQUIRE(v3.x == -3.0f);
    REQUIRE(v3.y == -3.0f);
    REQUIRE(v3.z == -3.0f);
}

TEST_CASE("Vec3f - operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    jtx::Vec3f v2 = v1 - s;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == 0.0f);
    REQUIRE(v2.z == 1.0f);

}

TEST_CASE("Vec3f * operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = v1 * v2;
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 10.0f);
    REQUIRE(v3.z == 18.0f);
}

TEST_CASE("Vec3f * operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    jtx::Vec3f v2 = v1 * s;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 6.0f);
}

TEST_CASE("Vec3f * operator (LHS scalar)", "[Vec3]") {
    float s = 2.0f;
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2 = s * v1;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 6.0f);
}

TEST_CASE("Vec3f / operator", "[Vec3]") {
    jtx::Vec3f v1(4.0f, 10.0f, 18.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = v1 / v2;
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 2.0f);
    REQUIRE(v3.z == 3.0f);
}

TEST_CASE("Vec3f / operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(2.0f, 4.0f, 6.0f);
    float s = 2.0f;
    jtx::Vec3f v2 = v1 / s;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f / operator (LHS scalar)", "[Vec3]") {
    float s = 2.0f;
    jtx::Vec3f v1(2.0f, 4.0f, 6.0f);
    jtx::Vec3f v2 = s / v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 0.5f);
    REQUIRE(v2.z == 1.0f / 3.0f);
}

TEST_CASE("Vec3f [] operator", "[Vec3]") {
    jtx::Vec3f v = {1.0f, 2.0f, 3.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
}
//endregion

//region Vec3: In-place operators
TEST_CASE("Vec3f += operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    v1 += v2;
    REQUIRE(v1.x == 5.0f);
    REQUIRE(v1.y == 7.0f);
    REQUIRE(v1.z == 9.0f);
}

TEST_CASE("Vec3f += operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    v1 += s;
    REQUIRE(v1.x == 3.0f);
    REQUIRE(v1.y == 4.0f);
    REQUIRE(v1.z == 5.0f);
}

TEST_CASE("Vec3f -= operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    v1 -= v2;
    REQUIRE(v1.x == -3.0f);
    REQUIRE(v1.y == -3.0f);
    REQUIRE(v1.z == -3.0f);
}

TEST_CASE("Vec3f -= operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    v1 -= s;
    REQUIRE(v1.x == -1.0f);
    REQUIRE(v1.y == 0.0f);
    REQUIRE(v1.z == 1.0f);
}

TEST_CASE("Vec3f *= operator", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    v1 *= v2;
    REQUIRE(v1.x == 4.0f);
    REQUIRE(v1.y == 10.0f);
    REQUIRE(v1.z == 18.0f);
}

TEST_CASE("Vec3f *= operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    float s = 2.0f;
    v1 *= s;
    REQUIRE(v1.x == 2.0f);
    REQUIRE(v1.y == 4.0f);
    REQUIRE(v1.z == 6.0f);
}

TEST_CASE("Vec3f /= operator", "[Vec3]") {
    jtx::Vec3f v1(4.0f, 10.0f, 18.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    v1 /= v2;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
}

TEST_CASE("Vec3f /= operator (scalar)", "[Vec3]") {
    jtx::Vec3f v1(2.0f, 4.0f, 6.0f);
    float s = 2.0f;
    v1 /= s;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
}
//endregion

//region Vec3: Functions
TEST_CASE("Vec3f cross (static)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = jtx::cross(v1, v2);
    REQUIRE(v3.x == -3.0f);
    REQUIRE(v3.y == 6.0f);
    REQUIRE(v3.z == -3.0f);
}

TEST_CASE("Vec3f cross", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3 = v1.cross(v2);
    REQUIRE(v3.x == -3.0f);
    REQUIRE(v3.y == 6.0f);
    REQUIRE(v3.z == -3.0f);
}

TEST_CASE("Vec3f dot", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    auto dot = v1.dot(v2);
    REQUIRE(dot == 32.0f);
}

TEST_CASE("Vec3f dot (with individual components)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    auto dot = v1.dot(4.0f, 5.0f, 6.0f);
    REQUIRE(dot == 32.0f);
}

TEST_CASE("Vec3f len", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto len = v.len();
    REQUIRE_THAT(len, Catch::Matchers::WithinRel(3.741657f, T_EPS));
}

TEST_CASE("Vec3f normalize", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    v.normalize();
    REQUIRE_THAT(v.x, Catch::Matchers::WithinRel(0.267261f, T_EPS));
    REQUIRE_THAT(v.y, Catch::Matchers::WithinRel(0.534522f, T_EPS));
    REQUIRE_THAT(v.z, Catch::Matchers::WithinRel(0.801784f, T_EPS));
}

TEST_CASE("Vec3f normalize (static)", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto v2 = jtx::normalize(v);
    REQUIRE_THAT(v2.x, Catch::Matchers::WithinRel(0.267261f, T_EPS));
    REQUIRE_THAT(v2.y, Catch::Matchers::WithinRel(0.534522f, T_EPS));
    REQUIRE_THAT(v2.z, Catch::Matchers::WithinRel(0.801784f, T_EPS));
}

TEST_CASE("Vec3f abs", "[Vec3]") {
    jtx::Vec3f v(-1.0f, -2.0f, -3.0f);
    v.abs();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vec3f abs (static)", "[Vec3]") {
    jtx::Vec3f v(-1.0f, -2.0f, -3.0f);
    auto v2 = jtx::abs(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f absdot", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(-4.0f, -5.0f, -6.0f);
    auto dot = v1.absdot(v2);
    REQUIRE(dot == 32.0f);
}

TEST_CASE("Vec3f absdot (static)", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(-4.0f, -5.0f, -6.0f);
    auto dot = jtx::absdot(v1, v2);
    REQUIRE(dot == 32.0f);

}

TEST_CASE("Vec3f ceil", "[Vec3]") {
    jtx::Vec3f v(1.1f, 2.2f, 3.3f);
    v.ceil();
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
}

TEST_CASE("Vec3f ceil (static)", "[Vec3]") {
    jtx::Vec3f v(1.1f, 2.2f, 3.3f);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
    REQUIRE(v2.z == 4.0f);
}

TEST_CASE("Vec3f floor", "[Vec3]") {
    jtx::Vec3f v(1.1f, 2.2f, 3.3f);
    v.floor();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vec3f floor (static)", "[Vec3]") {
    jtx::Vec3f v(1.1f, 2.2f, 3.3f);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
}

TEST_CASE("Vec3f min", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto min = v.min();
    REQUIRE(min == 1.0f);
}

TEST_CASE("Vec3f min two vectors", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 1.0f, 2.0f);
    auto v3 = jtx::min(v1, v2);
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 1.0f);
    REQUIRE(v3.z == 2.0f);
}

TEST_CASE("Vec3f max", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto max = v.max();
    REQUIRE(max == 3.0f);
}

TEST_CASE("Vec3f max two vectors", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 1.0f, 2.0f);
    auto v3 = jtx::max(v1, v2);
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 2.0f);
    REQUIRE(v3.z == 3.0f);
}

TEST_CASE("Vec3f hprod", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto hprod = v.hprod();
    REQUIRE(hprod == 6.0f);
}

TEST_CASE("Vec3f hprod (static)", "[Vec3]") {
    jtx::Vec3f v(1.0f, 2.0f, 3.0f);
    auto hprod = jtx::hprod(v);
    REQUIRE(hprod == 6.0f);
}

TEST_CASE("Vec3f lerp", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    auto v3 = jtx::lerp(v1, v2, 0.5f);
    REQUIRE(v3.x == 2.5f);
    REQUIRE(v3.y == 3.5f);
    REQUIRE(v3.z == 4.5f);
}

TEST_CASE("Vec3f FMA", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3f v2(4.0f, 5.0f, 6.0f);
    jtx::Vec3f v3(7.0f, 8.0f, 9.0f);
    auto v4 = jtx::fma(v1, v2, v3);
    REQUIRE(v4.x == 11.0f);
    REQUIRE(v4.y == 18.0f);
    REQUIRE(v4.z == 27.0f);
}

TEST_CASE("Vec3f angle", "[Vec3]") {
    jtx::Vec3f up{0.0f, 1.0f, 0.0f};
    jtx::Vec3f forward{0.0f, 0.0f, 1.0f};
    auto theta = jtx::angle(up, forward);
    // 90 degrees
    REQUIRE_THAT(theta, Catch::Matchers::WithinRel(T_DEG_90, T_EPS));
}

TEST_CASE("Vec3f L1 Norm", "[Vec3]") {
    jtx::Vec3f v{1.0f, 2.0f, 3.0f};
    auto norm = v.l1norm();
    REQUIRE(norm == 6.0f);
}

// TODO: this appears to be backwards?!
TEST_CASE("Vec3f Gram Schmidt", "[Vec3]") {
    jtx::Vec3f b{1.0f, 0.0f, 0.0f};
    jtx::Vec3f a{1.0f, 1.0f, 0.0f};
    auto c = jtx::gramSchmidt(a, b);
    REQUIRE(c.x == 0.0f);
    REQUIRE(c.y == 1.0f);
    REQUIRE(c.z == 0.0f);
}

TEST_CASE("Vec3f Coordinate System", "[Vec3]") {
    jtx::Vec3f v1{1.0f, 0.0f, 0.0f};
    jtx::Vec3f v2{};
    jtx::Vec3f v3{};
    jtx::coordinateSystem(v1, &v2, &v3);

    REQUIRE(v2 == jtx::Vec3f{0.0f, 0.0f, -1.0f});
    REQUIRE(v3 == jtx::Vec3f{0.0f, 1.0f, 0.0f});
}

TEST_CASE("Point3f distance", "[Vec3]") {
    jtx::Point3f p1{1.0f, 2.0f, 3.0f};
    jtx::Point3f p2{4.0f, 5.0f, 6.0f};
    auto dist = jtx::distance(p1, p2);
    REQUIRE_THAT(dist, Catch::Matchers::WithinRel(5.196152f, T_EPS));
}

TEST_CASE ("Normal3f align", "[Vec3]") {
    jtx::Normal3f v1{1.0f, 2.0f, 3.0f};
    jtx::Normal3f v2{-1.0f, -2.0f, -3.0f};
    v1.align(v2);
    REQUIRE(v1.x == -1.0f);
    REQUIRE(v1.y == -2.0f);
    REQUIRE(v1.z == -3.0f);
}

TEST_CASE ("Normal3f align (static)", "[Vec3]") {
    jtx::Normal3f v1{1.0f, 2.0f, 3.0f};
    jtx::Normal3f v2{-1.0f, -2.0f, -3.0f};
    auto v3 = jtx::align(v1, v2);
    REQUIRE(v3.x == -1.0f);
    REQUIRE(v3.y == -2.0f);
    REQUIRE(v3.z == -3.0f);
}
//endregion

//region Vec3i Tests
TEST_CASE("Vec3i default constructor", "[Vec3]") {
    jtx::Vec3i v;
    REQUIRE(v.x == 0);
    REQUIRE(v.y == 0);
    REQUIRE(v.z == 0);
}

TEST_CASE("Vec3i template copy constructor", "[Vec3]") {
    jtx::Vec3f v1(1.0f, 2.0f, 3.0f);
    jtx::Vec3i v2(v1);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
}

TEST_CASE("Vec3i valid", "[Vec3]") {
    jtx::Vec3i v(1, 2, 3);
    REQUIRE(v.valid());
}

TEST_CASE("Vec3i ceil", "[Vec3]") {
    jtx::Vec3i v(1, 2, 3);
    v.ceil();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
    REQUIRE(v.z == 3);
}

TEST_CASE("Vec3i ceil (static)", "[Vec3]") {
    jtx::Vec3i v(1, 2, 3);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
}

TEST_CASE("Vec3i floor", "[Vec3]") {
    jtx::Vec3i v(1, 2, 3);
    v.floor();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
    REQUIRE(v.z == 3);
}

TEST_CASE("Vec3i floor (static)", "[Vec3]") {
    jtx::Vec3i v(1, 2, 3);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
}

TEST_CASE("Vec3i cross (static)", "[Vec3]") {
    jtx::Vec3i v1(1, 2, 3);
    jtx::Vec3i v2(4, 5, 6);
    jtx::Vec3i v3 = jtx::cross(v1, v2);
    REQUIRE(v3.x == -3);
    REQUIRE(v3.y == 6);
    REQUIRE(v3.z == -3);
}
//endregion
