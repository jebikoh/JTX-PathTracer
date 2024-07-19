#include <jtxlib/math/spherical.hpp>
#include "./tconstants.h"
#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

//region Spherical Area
TEST_CASE("sphericalTriangleArea", "[spherical]") {
    jtx::Vec3f a(1.0f, 0.0f, 0.0f);
    jtx::Vec3f b(0.0f, 1.0f, 0.0f);
    jtx::Vec3f c(0.0f, 0.0f, 1.0f);

    REQUIRE_THAT(jtx::sphericalTriangleArea(a, b, c), Catch::Matchers::WithinRel(jtx::PI_F / 2, T_EPS));
}

TEST_CASE("sphericalQuadArea", "[spherical]") {
    jtx::Vec3f a(1.0f, 0.0f, 0.0f);
    jtx::Vec3f b(0.0f, 1.0f, 0.0f);
    jtx::Vec3f c(-1.0f, 0.0f, 0.0f);
    jtx::Vec3f d(0.0f, -1.0f, 0.0f);

    REQUIRE_THAT(jtx::sphericalQuadArea(a, b, c, d), Catch::Matchers::WithinRel(jtx::PI_F * 2, T_EPS));
}
//endregion

//region OctahedralVec Tests
TEST_CASE("OctahedaralVec conserves unit vector", "[Octahedral]") {
    jtx::Vec3f v{1.0f, 2.0f, 3.0f};
    v.normalize();
    jtx::OctahedralVec oct(v);
    auto v2 = jtx::Vec3f(oct);
    REQUIRE_THAT(v2.x, Catch::Matchers::WithinRel(v.x, T_EPS));
    REQUIRE_THAT(v2.y, Catch::Matchers::WithinRel(v.y, T_EPS));
    REQUIRE_THAT(v2.z, Catch::Matchers::WithinRel(v.z, T_EPS));
}
//endregion

//region Equal areas

TEST_CASE("Equal area square to sphere to square conversion", "[spherical]") {
    jtx::Vec2f a(1.0f, 1.0f);
    jtx::Vec2f b(0.0f, 0.0f);
    jtx::Vec2f c(1.0f, 0.0f);
    jtx::Vec2f d(0.0f, 1.0f);

    jtx::Vec3f aSphere = jtx::equalAreaSquareToSphere(a);
    jtx::Vec3f bSphere = jtx::equalAreaSquareToSphere(b);
    jtx::Vec3f cSphere = jtx::equalAreaSquareToSphere(c);
    jtx::Vec3f dSphere = jtx::equalAreaSquareToSphere(d);

    jtx::Vec2f aSquare = jtx::equalAreaSphereToSquare(aSphere);
    jtx::Vec2f bSquare = jtx::equalAreaSphereToSquare(bSphere);
    jtx::Vec2f cSquare = jtx::equalAreaSphereToSquare(cSphere);
    jtx::Vec2f dSquare = jtx::equalAreaSphereToSquare(dSphere);

    REQUIRE_THAT(aSquare.x, Catch::Matchers::WithinRel(a.x, T_EPS));
    REQUIRE_THAT(aSquare.y, Catch::Matchers::WithinRel(a.y, T_EPS));
    REQUIRE_THAT(bSquare.x, Catch::Matchers::WithinRel(b.x, T_EPS));
    REQUIRE_THAT(bSquare.y, Catch::Matchers::WithinRel(b.y, T_EPS));
    REQUIRE_THAT(cSquare.x, Catch::Matchers::WithinRel(c.x, T_EPS));
    REQUIRE_THAT(cSquare.y, Catch::Matchers::WithinRel(c.y, T_EPS));
    REQUIRE_THAT(dSquare.x, Catch::Matchers::WithinRel(d.x, T_EPS));
    REQUIRE_THAT(dSquare.y, Catch::Matchers::WithinRel(d.y, T_EPS));
}

//endregion