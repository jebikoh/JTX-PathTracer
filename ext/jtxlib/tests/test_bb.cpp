#include <jtxlib/math/bounds.hpp>
#include "tconstants.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <iostream>

//region BB3f Constructors
TEST_CASE("BB3f default constructor", "[BB3]") {
    jtx::BB3f bb;
    REQUIRE(bb.pmin.x == std::numeric_limits<float>::max());
    REQUIRE(bb.pmin.y == std::numeric_limits<float>::max());
    REQUIRE(bb.pmin.z == std::numeric_limits<float>::max());
    REQUIRE(bb.pmax.x == std::numeric_limits<float>::lowest());
    REQUIRE(bb.pmax.y == std::numeric_limits<float>::lowest());
    REQUIRE(bb.pmax.z == std::numeric_limits<float>::lowest());
}

TEST_CASE("BB3f constructor with 2 points", "[BB3]") {
    jtx::Point3f pmin(1.0f, 8.0f, 3.0f);
    jtx::Point3f pmax(4.0f, 5.0f, 6.0f);
    jtx::BB3f bb(pmin, pmax);
    REQUIRE(bb.pmin == jtx::Point3f{1.0f, 5.0f, 3.0f});
    REQUIRE(bb.pmax == jtx::Point3f{4.0f, 8.0f, 6.0f});
}

TEST_CASE("BB3f constructor with 1 point", "[BB3]") {
    jtx::Point3f p(1.0f, 2.0f, 3.0f);
    jtx::BB3f bb(p);
    REQUIRE(bb.pmin == p);
    REQUIRE(bb.pmax == p);
}

TEST_CASE("BB3f copy constructor", "[BB3]") {
    jtx::Point3f pmin(1.0f, 8.0f, 3.0f);
    jtx::Point3f pmax(4.0f, 5.0f, 6.0f);
    jtx::BB3f bb(pmin, pmax);
    jtx::BB3f bbCopy(bb);
    REQUIRE(bbCopy.pmin == jtx::Point3f{1.0f, 5.0f, 3.0f});
    REQUIRE(bbCopy.pmax == jtx::Point3f{4.0f, 8.0f, 6.0f});
}

TEST_CASE("BB3f copy constructor (empty, f->i)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmin = jtx::Point3f{1.0f, 1.0f, 1.0f};
    bb.pmax = jtx::Point3f{0.0f, 0.0f, 0.0f};
    REQUIRE(bb.isEmpty() == true);
    auto bb2 = jtx::BB3i(bb);
    REQUIRE(bb2.isEmpty() == true);
    REQUIRE(bb2.pmax.x == std::numeric_limits<int>::lowest());
    REQUIRE(bb2.pmax.y == std::numeric_limits<int>::lowest());
    REQUIRE(bb2.pmax.z == std::numeric_limits<int>::lowest());
    REQUIRE(bb2.pmin.x == std::numeric_limits<int>::max());
    REQUIRE(bb2.pmin.y == std::numeric_limits<int>::max());
    REQUIRE(bb2.pmin.z == std::numeric_limits<int>::max());
}

TEST_CASE("BB3f copy constructor (f->i)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmin = jtx::Point3f{0.0f, 0.0f, 0.0f};
    bb.pmax = jtx::Point3f{1.0f, 1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == false);
    auto bb2 = jtx::BB3i(bb);
    REQUIRE(bb2.isEmpty() == false);
    REQUIRE(bb2.pmax == jtx::Point3i{1, 1, 1});
    REQUIRE(bb2.pmin == jtx::Point3i{0, 0, 0});
}
//endregion

//region BB3f isEmpty() / isDegenerate()
TEST_CASE("BB3f isEmpty (true)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmax = jtx::Point3f{1.0f, 1.0f, 1.0f};
    bb.pmin = jtx::Point3f{2.0f, 2.0f, 2.0f};
    REQUIRE(bb.isEmpty() == true);
}

TEST_CASE("BB3f isEmpty (false)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmax = jtx::Point3f{2.0f, 2.0f, 2.0f};
    bb.pmin = jtx::Point3f{1.0f, 1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == false);
}

TEST_CASE("BB3f isEmpty (one-dim, true)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmax = jtx::Point3f{1.0f, 2.0f, 2.0f};
    bb.pmin = jtx::Point3f{1.0f, 1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == true);
}

TEST_CASE("BB3f isDegenerate (true)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmax = jtx::Point3f{1.0f, 1.0f, 1.0f};
    bb.pmin = jtx::Point3f{2.0f, 2.0f, 2.0f};
    REQUIRE(bb.isDegenerate() == true);
}

TEST_CASE("BB3f isDegenerate (false)", "[BB3]") {
    jtx::BB3f bb;
    bb.pmax = jtx::Point3f{3.0f, 3.0f, 3.0f};
    bb.pmin = jtx::Point3f{2.0f, 2.0f, 2.0f};
    REQUIRE(bb.isDegenerate() == false);
}
//endregion

//region BB3f Member functions
TEST_CASE("BB3f corner", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    REQUIRE(bb.corner(0) == jtx::Point3f{1.0f, 1.0f, 1.0f});
    REQUIRE(bb.corner(1) == jtx::Point3f{2.0f, 1.0f, 1.0f});
    REQUIRE(bb.corner(2) == jtx::Point3f{1.0f, 2.0f, 1.0f});
    REQUIRE(bb.corner(3) == jtx::Point3f{2.0f, 2.0f, 1.0f});
    REQUIRE(bb.corner(4) == jtx::Point3f{1.0f, 1.0f, 2.0f});
    REQUIRE(bb.corner(5) == jtx::Point3f{2.0f, 1.0f, 2.0f});
    REQUIRE(bb.corner(6) == jtx::Point3f{1.0f, 2.0f, 2.0f});
    REQUIRE(bb.corner(7) == jtx::Point3f{2.0f, 2.0f, 2.0f});
}

TEST_CASE("BB3f merge with BB3f", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{3.0f, 3.0f, 3.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    bb1.merge(bb2);
    REQUIRE(bb1.pmin == jtx::Point3f{1.0f, 1.0f, 1.0f});
    REQUIRE(bb1.pmax == jtx::Point3f{4.0f, 4.0f, 4.0f});
}

TEST_CASE("BB3f merge with Point3f", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(3.0f, 3.0f, 3.0f);
    bb.merge(p);
    REQUIRE(bb.pmin == jtx::Point3f{1.0f, 1.0f, 1.0f});
    REQUIRE(bb.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f overlaps: does overlap", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{1.5f, 1.5f, 1.5f}, jtx::Point3f{2.5f, 2.5f, 2.5f});
    REQUIRE(bb1.overlaps(bb2) == true);
}

TEST_CASE("BB3f overlaps: does not overlap", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{2.5f, 2.5f, 2.5f}, jtx::Point3f{3.5f, 3.5f, 3.5f});
    REQUIRE(bb1.overlaps(bb2) == false);
}

TEST_CASE("BB3f overlaps: equal edge", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{2.0f, 2.0f, 2.0f}, jtx::Point3f{2.5f, 2.5f, 2.5f});
    REQUIRE(bb1.overlaps(bb2) == true);
}

TEST_CASE("BB3f inside: true", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.5f, 1.5f, 1.5f);
    REQUIRE(bb.inside(p) == true);
}

TEST_CASE("BB3f inside: false", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(2.5f, 2.5f, 2.5f);
    REQUIRE(bb.inside(p) == false);
}

TEST_CASE("BB3f inside: true, on edge", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    REQUIRE(bb.inside(p) == true);
}

TEST_CASE("BB3f inside exclusive: false, on edge") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    REQUIRE(bb.insideExclusive(p) == false);
}

TEST_CASE("BB3f inside exclusive, true", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.5f, 1.5f, 1.5f);
    REQUIRE(bb.insideExclusive(p) == true);
}

TEST_CASE("BB3f expand", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    bb.expand(1.0f);
    REQUIRE(bb.pmin == jtx::Point3f{0.0f, 0.0f, 0.0f});
    REQUIRE(bb.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f shrink", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    bb.shrink(1.0f);
    REQUIRE(bb.pmin == jtx::Point3f{2.0f, 2.0f, 2.0f});
    REQUIRE(bb.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f diagonal", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    REQUIRE(bb.diagonal() == jtx::Vec3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f surface area", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    REQUIRE(bb.surfaceArea() == 54.0f);
}

TEST_CASE("BB3f volume", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    REQUIRE(bb.volume() == 27.0f);
}

TEST_CASE("BB3f maxDim", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 5.0f});
    REQUIRE(bb.maxDim() == 2);
}

TEST_CASE("BB3f lerp", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 5.0f});
    jtx::Point3f t(0.5f, 0.5f, 0.5f);
    REQUIRE(bb.lerp(t) == jtx::Point3f{2.5f, 2.5f, 3.0f});
}

TEST_CASE("BB3f offset: middle", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.5f, 1.5f, 1.5f);
    REQUIRE(bb.offset(p) == jtx::Vec3f{0.5f, 0.5f, 0.5f});
}

TEST_CASE("BB3f offset: min", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    REQUIRE(bb.offset(p) == jtx::Vec3f{0.0f, 0.0f, 0.0f});
}

TEST_CASE("BB3f offset: max", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(2.0f, 2.0f, 2.0f);
    REQUIRE(bb.offset(p) == jtx::Vec3f{1.0f, 1.0f, 1.0f});
}

TEST_CASE("BB3f offset: out-of-bounds", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(3.0f, 3.0f, 3.0f);
    REQUIRE(bb.offset(p) == jtx::Vec3f{2.0f, 2.0f, 2.0f});
}

TEST_CASE("BB3f bounding sphere", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{3.0f, 3.0f, 3.0f});
    jtx::Point3f center;
    float radius;
    bb.boundingSphere(&center, &radius);
    REQUIRE(center == jtx::Point3f{2.0f, 2.0f, 2.0f});
    REQUIRE(radius == std::sqrt(3.0f));
}
//endregion

//region BB3f static functions
TEST_CASE("BB3f merge with BB3f (static)", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{3.0f, 3.0f, 3.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    auto bb3 = jtx::merge(bb1, bb2);
    REQUIRE(bb3.pmin == jtx::Point3f{1.0f, 1.0f, 1.0f});
    REQUIRE(bb3.pmax == jtx::Point3f{4.0f, 4.0f, 4.0f});
}

TEST_CASE("BB3f merge with Point3f (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(3.0f, 3.0f, 3.0f);
    auto bb2 = jtx::merge(bb, p);
    REQUIRE(bb2.pmin == jtx::Point3f{1.0f, 1.0f, 1.0f});
    REQUIRE(bb2.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f intersect", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{3.0f, 3.0f, 3.0f});
    jtx::BB3f bb2(jtx::Point3f{2.0f, 2.0f, 2.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    auto bb3 = jtx::intersect(bb1, bb2);
    REQUIRE(bb3.pmin == jtx::Point3f{2.0f, 2.0f, 2.0f});
    REQUIRE(bb3.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f overlaps: does overlap (static)", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{1.5f, 1.5f, 1.5f}, jtx::Point3f{2.5f, 2.5f, 2.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == true);
}

TEST_CASE("BB3f overlaps: does not overlap (static)", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{2.5f, 2.5f, 2.5f}, jtx::Point3f{3.5f, 3.5f, 3.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == false);
}

TEST_CASE("BB3f overlaps: equal edge (static)", "[BB3f]") {
    jtx::BB3f bb1(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::BB3f bb2(jtx::Point3f{2.0f, 2.0f, 2.0f}, jtx::Point3f{2.5f, 2.5f, 2.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == true);
}

TEST_CASE("BB3f inside: true (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.5f, 1.5f, 1.5f);
    REQUIRE(jtx::inside(p, bb) == true);
}

TEST_CASE("BB3f inside: false (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(2.5f, 2.5f, 2.5f);
    REQUIRE(jtx::inside(p, bb) == false);
}

TEST_CASE("BB3f inside: true, on edge (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    REQUIRE(jtx::inside(p, bb) == true);
}

TEST_CASE("BB3f inside exclusive: false, on edge (static)") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    REQUIRE(jtx::insideExclusive(p, bb) == false);
}

TEST_CASE("BB3f inside exclusive, true (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    jtx::Point3f p(1.5f, 1.5f, 1.5f);
    REQUIRE(jtx::insideExclusive(p, bb) == true);
}

TEST_CASE("BB3f expand (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{2.0f, 2.0f, 2.0f});
    auto bb2 = jtx::expand(bb, 1.0f);
    REQUIRE(bb2.pmin == jtx::Point3f{0.0f, 0.0f, 0.0f});
    REQUIRE(bb2.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f shrink (static)", "[BB3f]") {
    jtx::BB3f bb(jtx::Point3f{1.0f, 1.0f, 1.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    auto bb2 = jtx::shrink(bb, 1.0f);
    REQUIRE(bb2.pmin == jtx::Point3f{2.0f, 2.0f, 2.0f});
    REQUIRE(bb2.pmax == jtx::Point3f{3.0f, 3.0f, 3.0f});
}

TEST_CASE("BB3f distance", "[BB3f]") {
    jtx::Point3f p(1.0f, 1.0f, 1.0f);
    jtx::BB3f bb(jtx::Point3f{2.0f, 2.0f, 2.0f}, jtx::Point3f{4.0f, 4.0f, 4.0f});
    REQUIRE_THAT(jtx::distance(bb, p), Catch::Matchers::WithinRel(1.732050776f, T_EPS));
}
//endregion

//region BB2f Constructors
TEST_CASE("BB2f default constructor", "[BB3]") {
    jtx::BB2f bb;
    REQUIRE(bb.pmin.x == std::numeric_limits<float>::max());
    REQUIRE(bb.pmin.y == std::numeric_limits<float>::max());
    REQUIRE(bb.pmax.x == std::numeric_limits<float>::lowest());
    REQUIRE(bb.pmax.y == std::numeric_limits<float>::lowest());
}

TEST_CASE("BB2f constructor with 2 points", "[BB3]") {
    jtx::Point2f pmin(1.0f, 8.0f);
    jtx::Point2f pmax(4.0f, 5.0f);
    jtx::BB2f bb(pmin, pmax);
    REQUIRE(bb.pmin == jtx::Point2f{1.0f, 5.0f});
    REQUIRE(bb.pmax == jtx::Point2f{4.0f, 8.0f});
}

TEST_CASE("BB2f constructor with 1 point", "[BB3]") {
    jtx::Point2f p(1.0f, 2.0f);
    jtx::BB2f bb(p);
    REQUIRE(bb.pmin == p);
    REQUIRE(bb.pmax == p);
}

TEST_CASE("BB2f copy constructor", "[BB3]") {
    jtx::Point2f pmin(1.0f, 8.0f);
    jtx::Point2f pmax(4.0f, 5.0f);
    jtx::BB2f bb(pmin, pmax);
    jtx::BB2f bbCopy(bb);
    REQUIRE(bbCopy.pmin == jtx::Point2f{1.0f, 5.0f});
    REQUIRE(bbCopy.pmax == jtx::Point2f{4.0f, 8.0f});
}

TEST_CASE("BB2f copy constructor (empty, f->i)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmin = jtx::Point2f{1.0f, 1.0f};
    bb.pmax = jtx::Point2f{0.0f, 0.0f};
    REQUIRE(bb.isEmpty() == true);
    auto bb2 = jtx::BB2i(bb);
    REQUIRE(bb2.isEmpty() == true);
    REQUIRE(bb2.pmax.x == std::numeric_limits<int>::lowest());
    REQUIRE(bb2.pmax.y == std::numeric_limits<int>::lowest());
    REQUIRE(bb2.pmin.x == std::numeric_limits<int>::max());
    REQUIRE(bb2.pmin.y == std::numeric_limits<int>::max());
}

TEST_CASE("BB2f copy constructor (f->i)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmin = jtx::Point2f{0.0f, 0.0f};
    bb.pmax = jtx::Point2f{1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == false);
    auto bb2 = jtx::BB2i(bb);
    REQUIRE(bb2.isEmpty() == false);
    REQUIRE(bb2.pmax == jtx::Point2i{1, 1});
    REQUIRE(bb2.pmin == jtx::Point2i{0, 0});
}
//endregion

//region BB2f isEmpty() / isDegenerate()
TEST_CASE("BB2f isEmpty (true)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmax = jtx::Point2f{1.0f, 1.0f};
    bb.pmin = jtx::Point2f{2.0f, 2.0f};
    REQUIRE(bb.isEmpty() == true);
}

TEST_CASE("BB2f isEmpty (false)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmax = jtx::Point2f{2.0f, 2.0f};
    bb.pmin = jtx::Point2f{1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == false);
}

TEST_CASE("BB2f isEmpty (one-dim, true)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmax = jtx::Point2f{1.0f, 2.0f};
    bb.pmin = jtx::Point2f{1.0f, 1.0f};
    REQUIRE(bb.isEmpty() == true);
}

TEST_CASE("BB2f isDegenerate (true)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmax = jtx::Point2f{1.0f, 1.0f};
    bb.pmin = jtx::Point2f{2.0f, 2.0f};
    REQUIRE(bb.isDegenerate() == true);
}

TEST_CASE("BB2f isDegenerate (false)", "[BB3]") {
    jtx::BB2f bb;
    bb.pmax = jtx::Point2f{3.0f, 3.0f};
    bb.pmin = jtx::Point2f{2.0f, 2.0f};
    REQUIRE(bb.isDegenerate() == false);
}
//endregion

//region BB2f Member functions
TEST_CASE("BB2f corner", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    REQUIRE(bb.corner(0) == jtx::Point2f{1.0f, 1.0f});
    REQUIRE(bb.corner(1) == jtx::Point2f{2.0f, 1.0f});
    REQUIRE(bb.corner(2) == jtx::Point2f{1.0f, 2.0f});
    REQUIRE(bb.corner(3) == jtx::Point2f{2.0f, 2.0f});
}

TEST_CASE("BB2f merge with BB2f", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{3.0f, 3.0f}, jtx::Point2f{4.0f, 4.0f});
    bb1.merge(bb2);
    REQUIRE(bb1.pmin == jtx::Point2f{1.0f, 1.0f});
    REQUIRE(bb1.pmax == jtx::Point2f{4.0f, 4.0f});
}

TEST_CASE("BB2f merge with Point2f", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(3.0f, 3.0f);
    bb.merge(p);
    REQUIRE(bb.pmin == jtx::Point2f{1.0f, 1.0f});
    REQUIRE(bb.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f overlaps: does overlap", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{1.5f, 1.5f}, jtx::Point2f{2.5f, 2.5f});
    REQUIRE(bb1.overlaps(bb2) == true);
}

TEST_CASE("BB2f overlaps: does not overlap", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{2.5f, 2.5f}, jtx::Point2f{3.5f, 3.5f});
    REQUIRE(bb1.overlaps(bb2) == false);
}

TEST_CASE("BB2f overlaps: equal edge", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{2.0f, 2.0f}, jtx::Point2f{2.5f, 2.5f});
    REQUIRE(bb1.overlaps(bb2) == true);
}

TEST_CASE("BB2f inside: true", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.5f, 1.5f);
    REQUIRE(bb.inside(p) == true);
}

TEST_CASE("BB2f inside: false", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(2.5f, 2.5f);
    REQUIRE(bb.inside(p) == false);
}

TEST_CASE("BB2f inside: true, on edge", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.0f, 1.0f);
    REQUIRE(bb.inside(p) == true);
}

TEST_CASE("BB2f inside exclusive: false, on edge") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.0f, 1.0f);
    REQUIRE(bb.insideExclusive(p) == false);
}

TEST_CASE("BB2f inside exclusive, true", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.5f, 1.5f);
    REQUIRE(bb.insideExclusive(p) == true);
}

TEST_CASE("BB2f expand", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    bb.expand(1.0f);
    REQUIRE(bb.pmin == jtx::Point2f{0.0f, 0.0f});
    REQUIRE(bb.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f shrink", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{4.0f, 4.0f});
    bb.shrink(1.0f);
    REQUIRE(bb.pmin == jtx::Point2f{2.0f, 2.0f});
    REQUIRE(bb.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f diagonal", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0}, jtx::Point2f{4.0f, 4.0f});
    REQUIRE(bb.diagonal() == jtx::Vec2f{3.0f, 3.0f});
}

TEST_CASE("BB2f maxDim", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{4.0f, 5.0f});
    REQUIRE(bb.maxDim() == 1);
}

TEST_CASE("BB2f lerp", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{4.0f, 4.0f});
    jtx::Point2f t(0.5f, 0.5f);
    REQUIRE(bb.lerp(t) == jtx::Point2f{2.5f, 2.5f});
}

TEST_CASE("BB2f offset: middle", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.5f, 1.5f);
    REQUIRE(bb.offset(p) == jtx::Vec2f{0.5f, 0.5f});
}

TEST_CASE("BB2f offset: min", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.0f, 1.0f);
    REQUIRE(bb.offset(p) == jtx::Vec2f{0.0f, 0.0f});
}

TEST_CASE("BB2f offset: max", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(2.0f, 2.0f);
    REQUIRE(bb.offset(p) == jtx::Vec2f{1.0f, 1.0f});
}

TEST_CASE("BB2f offset: out-of-bounds", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(3.0f, 3.0f);
    REQUIRE(bb.offset(p) == jtx::Vec2f{2.0f, 2.0f});
}
//endregion

//region BB2f static functions
TEST_CASE("BB2f merge with BB2f (static)", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{3.0f, 3.0f}, jtx::Point2f{4.0f, 4.0f});
    auto bb3 = jtx::merge(bb1, bb2);
    REQUIRE(bb3.pmin == jtx::Point2f{1.0f, 1.0f});
    REQUIRE(bb3.pmax == jtx::Point2f{4.0f, 4.0f});
}

TEST_CASE("BB2f merge with Point2f (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(3.0f, 3.0f);
    auto bb2 = jtx::merge(bb, p);
    REQUIRE(bb2.pmin == jtx::Point2f{1.0f, 1.0f});
    REQUIRE(bb2.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f intersect", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{3.0f, 3.0f});
    jtx::BB2f bb2(jtx::Point2f{2.0f, 2.0f}, jtx::Point2f{4.0f, 4.0f});
    auto bb3 = jtx::intersect(bb1, bb2);
    REQUIRE(bb3.pmin == jtx::Point2f{2.0f, 2.0f});
    REQUIRE(bb3.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f overlaps: does overlap (static)", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{1.5f, 1.5f}, jtx::Point2f{2.5f, 2.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == true);
}

TEST_CASE("BB2f overlaps: does not overlap (static)", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{2.5f, 2.5f}, jtx::Point2f{3.5f, 3.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == false);
}

TEST_CASE("BB2f overlaps: equal edge (static)", "[BB2f]") {
    jtx::BB2f bb1(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::BB2f bb2(jtx::Point2f{2.0f, 2.0f}, jtx::Point2f{2.5f, 2.5f});
    REQUIRE(jtx::overlaps(bb1, bb2) == true);
}

TEST_CASE("BB2f inside: true (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.5f, 1.5f);
    REQUIRE(jtx::inside(p, bb) == true);
}

TEST_CASE("BB2f inside: false (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(2.5f, 2.5f);
    REQUIRE(jtx::inside(p, bb) == false);
}

TEST_CASE("BB2f inside: true, on edge (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.0f, 1.0f);
    REQUIRE(jtx::inside(p, bb) == true);
}

TEST_CASE("BB2f inside exclusive: false, on edge (static)") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.0f, 1.0f);
    REQUIRE(jtx::insideExclusive(p, bb) == false);
}

TEST_CASE("BB2f inside exclusive, true (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    jtx::Point2f p(1.5f, 1.5f);
    REQUIRE(jtx::insideExclusive(p, bb) == true);
}

TEST_CASE("BB2f expand (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{2.0f, 2.0f});
    auto bb2 = jtx::expand(bb, 1.0f);
    REQUIRE(bb2.pmin == jtx::Point2f{0.0f, 0.0f});
    REQUIRE(bb2.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f shrink (static)", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{4.0f, 4.0f});
    auto bb2 = jtx::shrink(bb, 1.0f);
    REQUIRE(bb2.pmin == jtx::Point2f{2.0f, 2.0f});
    REQUIRE(bb2.pmax == jtx::Point2f{3.0f, 3.0f});
}

TEST_CASE("BB2f distance", "[BB2f]") {
    jtx::BB2f bb(jtx::Point2f{1.0f, 1.0f}, jtx::Point2f{4.0f, 4.0f});
    jtx::Point2f p(0.0f, 0.0f);
    REQUIRE_THAT(jtx::distance(bb, p), Catch::Matchers::WithinRel(1.4142136f, T_EPS));
}
//endregion