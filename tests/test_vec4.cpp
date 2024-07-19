#include <jtxlib/math/vec4.hpp>
#include <jtxlib/math/vecmath.hpp>
#include "./tconstants.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

//region Vec4: Constructors
TEST_CASE("Vec4f default constructor", "[Vec4]") {
    jtx::Vec4f v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
    REQUIRE(v.w == 0.0f);
}

TEST_CASE("Vec4f constructor", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.w == 4.0f);
}

TEST_CASE("Vec4f copy constructor", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(v1);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f template copy constructor", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4i v2(v1);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
    REQUIRE(v2.w == 4);
}
//endregion

//region Vec4: Numerical validity
TEST_CASE("Vec4f valid", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v.valid());
}

#ifdef JTX_TEST

TEST_CASE("Vec4f invalid", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 4.0f, std::numeric_limits<float>::quiet_NaN());
    REQUIRE(!v.valid());
}

#endif
//endregion

//region Vec4: Unary operators
TEST_CASE("Vec4f operator- on positive values", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = -v1;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == -2.0f);
    REQUIRE(v2.z == -3.0f);
    REQUIRE(v2.w == -4.0f);
}

TEST_CASE("Vec4f operator- on negative values", "[Vec4]") {
    jtx::Vec4f v1(-1.0f, -2.0f, -3.0f, -4.0f);
    jtx::Vec4f v2 = -v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f operator+", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = +v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f ++ postfix operator", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = v++;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
    REQUIRE(v.w == 5.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f ++ prefix operator", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = ++v;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
    REQUIRE(v.w == 5.0f);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
    REQUIRE(v2.z == 4.0f);
    REQUIRE(v2.w == 5.0f);
}

TEST_CASE("Vec4f -- postfix operator", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = v--;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v.z == 2.0f);
    REQUIRE(v.w == 3.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f -- prefix operator", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = --v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v.z == 2.0f);
    REQUIRE(v.w == 3.0f);
    REQUIRE(v2.x == 0.0f);
    REQUIRE(v2.y == 1.0f);
    REQUIRE(v2.z == 2.0f);
    REQUIRE(v2.w == 3.0f);
}
//endregion

//region Vec4: Binary operators
TEST_CASE("Vec4f = operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 4.0f);
    v2 = v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f == operator (equal)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v1 == v2);
}

TEST_CASE("Vec4f == operator (unequal)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(1.0f, 2.0f, 4.0f, 4.0f);
    REQUIRE(!(v1 == v2));
}

TEST_CASE("Vec4f != operator (equal)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(!(v1 != v2));
}

TEST_CASE("Vec4f != operator (unequal)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(1.0f, 2.0f, 4.0f, 4.0f);
    REQUIRE(v1 != v2);
}

TEST_CASE("Vec4f + operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    jtx::Vec4f v3 = v1 + v2;
    REQUIRE(v3.x == 5.0f);
    REQUIRE(v3.y == 7.0f);
    REQUIRE(v3.z == 9.0f);
    REQUIRE(v3.w == 11.0f);
}

TEST_CASE("Vec4f + operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    jtx::Vec4f v2 = v1 + s;
    REQUIRE(v2.x == 3.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 5.0f);
    REQUIRE(v2.w == 6.0f);
}

TEST_CASE("Vec4f - operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    jtx::Vec4f v3 = v1 - v2;
    REQUIRE(v3.x == -3.0f);
    REQUIRE(v3.y == -3.0f);
    REQUIRE(v3.z == -3.0f);
    REQUIRE(v3.w == -3.0f);
}

TEST_CASE("Vec4f - operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    jtx::Vec4f v2 = v1 - s;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == 0.0f);
    REQUIRE(v2.z == 1.0f);
    REQUIRE(v2.w == 2.0f);
}

TEST_CASE("Vec4f * operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    jtx::Vec4f v3 = v1 * v2;
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 10.0f);
    REQUIRE(v3.z == 18.0f);
    REQUIRE(v3.w == 28.0f);
}

TEST_CASE("Vec4f * operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    jtx::Vec4f v2 = v1 * s;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 6.0f);
    REQUIRE(v2.w == 8.0f);
}

TEST_CASE("Vec4f * operator (LHS scalar)", "[Vec4]") {
    float s = 2.0f;
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2 = s * v1;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
    REQUIRE(v2.z == 6.0f);
    REQUIRE(v2.w == 8.0f);
}

TEST_CASE("Vec4f / operator", "[Vec4]") {
    jtx::Vec4f v1(4.0f, 10.0f, 18.0f, 28.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    jtx::Vec4f v3 = v1 / v2;
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 2.0f);
    REQUIRE(v3.z == 3.0f);
    REQUIRE(v3.w == 4.0f);
}

TEST_CASE("Vec4f / operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(2.0f, 4.0f, 6.0f, 8.0f);
    float s = 2.0f;
    jtx::Vec4f v2 = v1 / s;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f / operator (LHS scalar)", "[Vec4]") {
    float s = 8.0f;
    jtx::Vec4f v1(2.0f, 4.0f, 8.0f, 16.0f);
    jtx::Vec4f v2 = s / v1;
    REQUIRE(v2.x == 4.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 1.0f);
    REQUIRE(v2.w == 0.5f);
}

TEST_CASE("Vec4f [] operator", "[Vec4]") {
    jtx::Vec4f v = {1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
    REQUIRE(v[3] == 4.0f);
}
//endregion

//region Vec4: In-place operators
TEST_CASE("Vec4f += operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    v1 += v2;
    REQUIRE(v1.x == 5.0f);
    REQUIRE(v1.y == 7.0f);
    REQUIRE(v1.z == 9.0f);
    REQUIRE(v1.w == 11.0f);
}

TEST_CASE("Vec4f += operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    v1 += s;
    REQUIRE(v1.x == 3.0f);
    REQUIRE(v1.y == 4.0f);
    REQUIRE(v1.z == 5.0f);
    REQUIRE(v1.w == 6.0f);
}

TEST_CASE("Vec4f -= operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    v1 -= v2;
    REQUIRE(v1.x == -3.0f);
    REQUIRE(v1.y == -3.0f);
    REQUIRE(v1.z == -3.0f);
    REQUIRE(v1.w == -3.0f);
}

TEST_CASE("Vec4f -= operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    v1 -= s;
    REQUIRE(v1.x == -1.0f);
    REQUIRE(v1.y == 0.0f);
    REQUIRE(v1.z == 1.0f);
    REQUIRE(v1.w == 2.0f);
}

TEST_CASE("Vec4f *= operator", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    v1 *= v2;
    REQUIRE(v1.x == 4.0f);
    REQUIRE(v1.y == 10.0f);
    REQUIRE(v1.z == 18.0f);
    REQUIRE(v1.w == 28.0f);
}

TEST_CASE("Vec4f *= operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    float s = 2.0f;
    v1 *= s;
    REQUIRE(v1.x == 2.0f);
    REQUIRE(v1.y == 4.0f);
    REQUIRE(v1.z == 6.0f);
    REQUIRE(v1.w == 8.0f);
}

TEST_CASE("Vec4f /= operator", "[Vec4]") {
    jtx::Vec4f v1(4.0f, 10.0f, 18.0f, 28.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    v1 /= v2;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
    REQUIRE(v1.w == 4.0f);
}

TEST_CASE("Vec4f /= operator (scalar)", "[Vec4]") {
    jtx::Vec4f v1(2.0f, 4.0f, 6.0f, 8.0f);
    float s = 2.0f;
    v1 /= s;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
    REQUIRE(v1.z == 3.0f);
    REQUIRE(v1.w == 4.0f);
}
//endregion

//region Vec4: Functions

TEST_CASE("Vec4f dot", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    auto dot = v1.dot(v2);
    REQUIRE(dot == 60.0f);
}

TEST_CASE("Vec4f dot (with individual components)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    auto dot = v1.dot(4.0f, 5.0f, 6.0f, 7.0f);
    REQUIRE(dot == 60.0f);
}

TEST_CASE("Vec4f len", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto len = v.len();
    REQUIRE_THAT(len, Catch::Matchers::WithinRel(5.477225f, T_EPS));
}

TEST_CASE("Vec4f normalize", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    v.normalize();
    REQUIRE_THAT(v.x, Catch::Matchers::WithinRel(0.182574f, T_EPS));
    REQUIRE_THAT(v.y, Catch::Matchers::WithinRel(0.365148f, T_EPS));
    REQUIRE_THAT(v.z, Catch::Matchers::WithinRel(0.547722f, T_EPS));
    REQUIRE_THAT(v.w, Catch::Matchers::WithinRel(0.730296f, T_EPS));
}

TEST_CASE("Vec4f normalize (static)", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto v2 = jtx::normalize(v);
    REQUIRE_THAT(v2.x, Catch::Matchers::WithinRel(0.182574f, T_EPS));
    REQUIRE_THAT(v2.y, Catch::Matchers::WithinRel(0.365148f, T_EPS));
    REQUIRE_THAT(v2.z, Catch::Matchers::WithinRel(0.547722f, T_EPS));
    REQUIRE_THAT(v2.w, Catch::Matchers::WithinRel(0.730296f, T_EPS));
}

TEST_CASE("Vec4f abs", "[Vec4]") {
    jtx::Vec4f v(-1.0f, -2.0f, -3.0f, -4.0f);
    v.abs();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.w == 4.0f);
}

TEST_CASE("Vec4f abs (static)", "[Vec4]") {
    jtx::Vec4f v(-1.0f, -2.0f, -3.0f, -4.0f);
    auto v2 = jtx::abs(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f absdot", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(-4.0f, -5.0f, -6.0f, -7.0f);
    auto dot = v1.absdot(v2);
    REQUIRE(dot == 60.0f);
}

TEST_CASE("Vec4f absdot (static)", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(-4.0f, -5.0f, -6.0f, -7.0f);
    auto dot = jtx::absdot(v1, v2);
    REQUIRE(dot == 60.0f);

}

TEST_CASE("Vec4f ceil", "[Vec4]") {
    jtx::Vec4f v(1.1f, 2.2f, 3.3f, 4.4f);
    v.ceil();
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);
    REQUIRE(v.w == 5.0f);
}

TEST_CASE("Vec4f ceil (static)", "[Vec4]") {
    jtx::Vec4f v(1.1f, 2.2f, 3.3f, 4.4f);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
    REQUIRE(v2.z == 4.0f);
    REQUIRE(v2.w == 5.0f);
}

TEST_CASE("Vec4f floor", "[Vec4]") {
    jtx::Vec4f v(1.1f, 2.2f, 3.3f, 4.4f);
    v.floor();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.w == 4.0f);
}

TEST_CASE("Vec4f floor (static)", "[Vec4]") {
    jtx::Vec4f v(1.1f, 2.2f, 3.3f, 4.4f);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
    REQUIRE(v2.z == 3.0f);
    REQUIRE(v2.w == 4.0f);
}

TEST_CASE("Vec4f min", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto min = v.min();
    REQUIRE(min == 1.0f);
}

TEST_CASE("Vec4f min two vectors", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 1.0f, 2.0f, 5.0f);
    auto v3 = jtx::min(v1, v2);
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 1.0f);
    REQUIRE(v3.z == 2.0f);
    REQUIRE(v3.w == 4.0f);
}

TEST_CASE("Vec4f max", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto max = v.max();
    REQUIRE(max == 4.0f);
}

TEST_CASE("Vec4f max two vectors", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 1.0f, 2.0f, 5.0f);
    auto v3 = jtx::max(v1, v2);
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 2.0f);
    REQUIRE(v3.z == 3.0f);
    REQUIRE(v3.w == 5.0f);
}

TEST_CASE("Vec4f hprod", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto hprod = v.hprod();
    REQUIRE(hprod == 24.0f);
}

TEST_CASE("Vec4f hprod (static)", "[Vec4]") {
    jtx::Vec4f v(1.0f, 2.0f, 3.0f, 4.0f);
    auto hprod = jtx::hprod(v);
    REQUIRE(hprod == 24.0f);
}

TEST_CASE("Vec4f lerp", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    auto v3 = jtx::lerp(v1, v2, 0.5f);
    REQUIRE(v3.x == 2.5f);
    REQUIRE(v3.y == 3.5f);
    REQUIRE(v3.z == 4.5f);
    REQUIRE(v3.w == 5.5f);
}

TEST_CASE("Vec4f FMA", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4f v2(4.0f, 5.0f, 6.0f, 7.0f);
    jtx::Vec4f v3(7.0f, 8.0f, 9.0f, 10.0f);
    auto v4 = jtx::fma(v1, v2, v3);
    REQUIRE(v4.x == 11.0f);
    REQUIRE(v4.y == 18.0f);
    REQUIRE(v4.z == 27.0f);
    REQUIRE(v4.w == 38.0f);
}
//endregion

//region Vec4i Tests
TEST_CASE("Vec4i default constructor", "[Vec4]") {
    jtx::Vec4i v;
    REQUIRE(v.x == 0);
    REQUIRE(v.y == 0);
    REQUIRE(v.z == 0);
    REQUIRE(v.w == 0);
}

TEST_CASE("Vec4i template copy constructor", "[Vec4]") {
    jtx::Vec4f v1(1.0f, 2.0f, 3.0f, 4.0f);
    jtx::Vec4i v2(v1);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
    REQUIRE(v2.w == 4);
}

TEST_CASE("Vec4i valid", "[Vec4]") {
    jtx::Vec4i v(1, 2, 3, 4);
    REQUIRE(v.valid());
}

TEST_CASE("Vec4i ceil", "[Vec4]") {
    jtx::Vec4i v(1, 2, 3, 4);
    v.ceil();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
    REQUIRE(v.z == 3);
    REQUIRE(v.w == 4);
}

TEST_CASE("Vec4i ceil (static)", "[Vec4]") {
    jtx::Vec4i v(1, 2, 3, 4);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
    REQUIRE(v2.w == 4);
}

TEST_CASE("Vec4i floor", "[Vec4]") {
    jtx::Vec4i v(1, 2, 3, 4);
    v.floor();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
    REQUIRE(v.z == 3);
    REQUIRE(v.w == 4);
}

TEST_CASE("Vec4i floor (static)", "[Vec4]") {
    jtx::Vec4i v(1, 2, 3, 4);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
    REQUIRE(v2.z == 3);
    REQUIRE(v2.w == 4);
}

//endregion