#include <jtxlib/math/vec2.hpp>
#include <jtxlib/math/vecmath.hpp>
#include "./tconstants.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>


//region Vec2: Constructors
TEST_CASE("Vec2f default constructor", "[Vec2]") {
    jtx::Vec2f v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
}

TEST_CASE("Vec2f constructor", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
}

TEST_CASE("Vec2f copy constructor", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(v1);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f template copy constructor", "[Vec2]") {
    jtx::Vec2i v1(1, 2);
    jtx::Vec2f v2(v1);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}
//endregion

//region Vec2: Numerical validity
TEST_CASE("Vec2f valid", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    REQUIRE(v.valid());
}

#ifdef JTX_TEST

TEST_CASE("Vec2f invalid", "[Vec2]") {
    jtx::Vec2f v(1.0f, std::numeric_limits<float>::quiet_NaN());
    REQUIRE(!v.valid());
}

#endif
//endregion

//region Vec2: Unary operators
TEST_CASE("Vec2f operator- on positive values", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2 = -v1;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == -2.0f);
}

TEST_CASE("Vec2f operator- on negative values", "[Vec2]") {
    jtx::Vec2f v1(-1.0f, -2.0f);
    jtx::Vec2f v2 = -v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f operator+", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2 = +v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f ++ postfix operator", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    jtx::Vec2f v2 = v++;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f ++ prefix operator", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    jtx::Vec2f v2 = ++v;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
}

TEST_CASE("Vec2f -- postfix operator", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    jtx::Vec2f v2 = v--;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f -- prefix operator", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    jtx::Vec2f v2 = --v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 1.0f);
    REQUIRE(v2.x == 0.0f);
    REQUIRE(v2.y == 1.0f);
}
//endregion

//region Vec2: Binary operators
TEST_CASE("Vec2f = operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    v2 = v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f == operator (equal)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(1.0f, 2.0f);
    REQUIRE(v1 == v2);
}

TEST_CASE("Vec2f == operator (unequal)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(1.0f, 3.0f);
    REQUIRE(!(v1 == v2));
}

TEST_CASE("Vec2f != operator (equal)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(1.0f, 2.0f);
    REQUIRE(!(v1 != v2));
}

TEST_CASE("Vec2f != operator (unequal)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(1.0f, 3.0f);
    REQUIRE(v1 != v2);
}

TEST_CASE("Vec2f + operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    jtx::Vec2f v3 = v1 + v2;
    REQUIRE(v3.x == 5.0f);
    REQUIRE(v3.y == 7.0f);
}

TEST_CASE("Vec2f + operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    jtx::Vec2f v2 = v1 + s;
    REQUIRE(v2.x == 3.0f);
    REQUIRE(v2.y == 4.0f);
}

TEST_CASE("Vec2f - operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    jtx::Vec2f v3 = v1 - v2;
    REQUIRE(v3.x == -3.0f);
    REQUIRE(v3.y == -3.0f);
}

TEST_CASE("Vec2f - scalar operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    jtx::Vec2f v2 = v1 - s;
    REQUIRE(v2.x == -1.0f);
    REQUIRE(v2.y == 0.0f);
}

TEST_CASE("Vec2f * operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    jtx::Vec2f v3 = v1 * v2;
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 10.0f);
}

TEST_CASE("Vec2f * operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    jtx::Vec2f v2 = v1 * s;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
}

TEST_CASE("Vec2f * operator (LHS scalar", "[Vec2]") {
    float s = 2.0f;
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2 = s * v1;
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 4.0f);
}

TEST_CASE("Vec2f / operator", "[Vec2]") {
    jtx::Vec2f v1(4.0f, 10.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    jtx::Vec2f v3 = v1 / v2;
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 2.0f);
}

TEST_CASE("Vec2f / operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(2.0f, 4.0f);
    float s = 2.0f;
    jtx::Vec2f v2 = v1 / s;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f / operator (LHS scalar)", "[Vec2]") {
    float s = 2.0f;
    jtx::Vec2f v1(2.0f, 4.0f);
    jtx::Vec2f v2 = s / v1;
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 0.5f);
}

TEST_CASE("Vec2f [] operator", "[Vec2]") {
    jtx::Vec2f v = {1.0f, 2.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
}
//endregion

//region Vec2: In-place operators
TEST_CASE("Vec2f += operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    v1 += v2;
    REQUIRE(v1.x == 5.0f);
    REQUIRE(v1.y == 7.0f);
}

TEST_CASE("Vec2f += operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    v1 += s;
    REQUIRE(v1.x == 3.0f);
    REQUIRE(v1.y == 4.0f);
}

TEST_CASE("Vec2f -= operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    v1 -= v2;
    REQUIRE(v1.x == -3.0f);
    REQUIRE(v1.y == -3.0f);
}

TEST_CASE("Vec2f -= operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    v1 -= s;
    REQUIRE(v1.x == -1.0f);
    REQUIRE(v1.y == 0.0f);

}

TEST_CASE("Vec2f *= operator", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    v1 *= v2;
    REQUIRE(v1.x == 4.0f);
    REQUIRE(v1.y == 10.0f);
}

TEST_CASE("Vec2f *= operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    float s = 2.0f;
    v1 *= s;
    REQUIRE(v1.x == 2.0f);
    REQUIRE(v1.y == 4.0f);
}

TEST_CASE("Vec2f /= operator", "[Vec2]") {
    jtx::Vec2f v1(4.0f, 10.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    v1 /= v2;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
}

TEST_CASE("Vec2f /= operator (scalar)", "[Vec2]") {
    jtx::Vec2f v1(2.0f, 4.0f);
    float s = 2.0f;
    v1 /= s;
    REQUIRE(v1.x == 1.0f);
    REQUIRE(v1.y == 2.0f);
}
//endregion

//region Vec2: Functions
TEST_CASE("Vec2f dot", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    auto dot = v1.dot(v2);
    REQUIRE(dot == 14.0f);
}

TEST_CASE("Vec2f dot (with individual components)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    auto dot = v1.dot(4.0f, 5.0f);
    REQUIRE(dot == 14.0f);
}

TEST_CASE("Vec2f len", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto len = v.len();
    REQUIRE_THAT(len, Catch::Matchers::WithinRel(2.236067f, T_EPS));
}

TEST_CASE("Vec2f normalize", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    v.normalize();
    REQUIRE_THAT(v.x, Catch::Matchers::WithinRel(0.447214f, T_EPS));
    REQUIRE_THAT(v.y, Catch::Matchers::WithinRel(0.894428f, T_EPS));
}

TEST_CASE("Vec2f normalize (static)", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto v2 = jtx::normalize(v);
    REQUIRE_THAT(v2.x, Catch::Matchers::WithinRel(0.447214f, T_EPS));
    REQUIRE_THAT(v2.y, Catch::Matchers::WithinRel(0.894428f, T_EPS));
}

TEST_CASE("Vec2f abs", "[Vec2]") {
    jtx::Vec2f v(-1.0f, -2.0f);
    v.abs();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
}

TEST_CASE("Vec2f abs (static)", "[Vec2]") {
    jtx::Vec2f v(-1.0f, -2.0f);
    auto v2 = jtx::abs(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f absdot", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(-4.0f, -5.0f);
    auto dot = v1.absdot(v2);
    REQUIRE(dot == 14.0f);
}

TEST_CASE("Vec2f absdot (static)", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(-4.0f, -5.0f);
    auto dot = jtx::absdot(v1, v2);
    REQUIRE(dot == 14.0f);
}

TEST_CASE("Vec2f ceil", "[Vec2]") {
    jtx::Vec2f v(1.1f, 2.2f);
    v.ceil();
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
}

TEST_CASE("Vec2f ceil (static)", "[Vec2]") {
    jtx::Vec2f v(1.1f, 2.2f);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 2.0f);
    REQUIRE(v2.y == 3.0f);
}

TEST_CASE("Vec2f floor", "[Vec2]") {
    jtx::Vec2f v(1.1f, 2.2f);
    v.floor();
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
}

TEST_CASE("Vec2f floor (static)", "[Vec2]") {
    jtx::Vec2f v(1.1f, 2.2f);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1.0f);
    REQUIRE(v2.y == 2.0f);
}

TEST_CASE("Vec2f min", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto min = v.min();
    REQUIRE(min == 1.0f);
}

TEST_CASE("Vec2f min two vectors", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 1.0f);
    auto v3 = jtx::min(v1, v2);
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 1.0f);
}

TEST_CASE("Vec2f max", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto max = v.max();
    REQUIRE(max == 2.0f);
}

TEST_CASE("Vec2f max two vectors", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 1.0f);
    auto v3 = jtx::max(v1, v2);
    REQUIRE(v3.x == 4.0f);
    REQUIRE(v3.y == 2.0f);
}

TEST_CASE("Vec2f hprod", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto hprod = v.hprod();
    REQUIRE(hprod == 2.0f);
}

TEST_CASE("Vec2f hprod (static)", "[Vec2]") {
    jtx::Vec2f v(1.0f, 2.0f);
    auto hprod = jtx::hprod(v);
    REQUIRE(hprod == 2.0f);
}

TEST_CASE("Vec2f lerp", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    auto v3 = jtx::lerp(v1, v2, 0.5f);
    REQUIRE(v3.x == 2.5f);
    REQUIRE(v3.y == 3.5f);
}

TEST_CASE("Vec2f FMA", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2f v2(4.0f, 5.0f);
    jtx::Vec2f v3(7.0f, 8.0f);
    auto v4 = jtx::fma(v1, v2, v3);
    REQUIRE(v4.x == 11.0f);
    REQUIRE(v4.y == 18.0f);
}

TEST_CASE("Vec2f angle", "[Vec2]") {
    jtx::Vec2f up{0.0f, 1.0f};
    jtx::Vec2f right{1.0f, 0.0f};
    auto theta = jtx::angle(up, right);
// 90 degrees
    REQUIRE_THAT(theta, Catch::Matchers::WithinRel(T_DEG_90, T_EPS));
}

// TODO: this appears to be backwards?!
TEST_CASE("Vec2f Gram Schmidt", "[Vec2]") {
    jtx::Vec2f b{1.0f, 0.0f};
    jtx::Vec2f a{1.0f, 1.0f};
    auto c = jtx::gramSchmidt(a, b);
    REQUIRE(c.x == 0.0f);
    REQUIRE(c.y == 1.0f);
}

TEST_CASE("Point2f distance", "[Vec2]") {
    jtx::Point2f p1{0.0f, 0.0f};
    jtx::Point2f p2{3.0f, 4.0f};
    auto d = jtx::distance(p1, p2);
    REQUIRE_THAT(d, Catch::Matchers::WithinRel(5.0f, T_EPS));
}
//endregion

//region Vec2i Tests
TEST_CASE("Vec2i default constructor", "[Vec2]") {
    jtx::Vec2i v;
    REQUIRE(v.x == 0);
    REQUIRE(v.y == 0);
}

TEST_CASE("Vec2i template copy constructor", "[Vec2]") {
    jtx::Vec2f v1(1.0f, 2.0f);
    jtx::Vec2i v2(v1);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
}

TEST_CASE("Vec2i valid", "[Vec2]") {
    jtx::Vec2i v(1, 2);
    REQUIRE(v.valid());
}

TEST_CASE("Vec2i ceil", "[Vec2]") {
    jtx::Vec2i v(1, 2);
    v.ceil();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
}

TEST_CASE("Vec2i ceil (static)", "[Vec2]") {
    jtx::Vec2i v(1, 2);
    auto v2 = jtx::ceil(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
}

TEST_CASE("Vec2i floor", "[Vec2]") {
    jtx::Vec2i v(1, 2);
    v.floor();
    REQUIRE(v.x == 1);
    REQUIRE(v.y == 2);
}

TEST_CASE("Vec2i floor (static)", "[Vec2]") {
    jtx::Vec2i v(1, 2);
    auto v2 = jtx::floor(v);
    REQUIRE(v2.x == 1);
    REQUIRE(v2.y == 2);
}

//endregion