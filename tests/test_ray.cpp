#include <jtxlib/math/ray.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("Ray default constructor", "[Ray]") {
    jtx::Ray ray;
    REQUIRE(ray.origin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.dir == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.time == 0.0f);
}

TEST_CASE("Ray constructor with parameters", "[Ray]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 7.0f;
    jtx::Ray ray(origin, dir, time);
    REQUIRE(ray.origin == origin);
    REQUIRE(ray.dir == dir);
    REQUIRE(ray.time == time);
}

TEST_CASE("Ray copy constructor", "[Ray]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 7.0f;
    jtx::Ray ray(origin, dir, time);
    jtx::Ray rayCopy(ray);
    REQUIRE(rayCopy.origin == origin);
    REQUIRE(rayCopy.dir == dir);
    REQUIRE(rayCopy.time == time);
}

TEST_CASE("Ray at", "[Ray]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 0.0f;
    jtx::Ray ray(origin, dir, time);
    float t = 1.0f;
    jtx::Point3f expected(5.0f, 7.0f, 9.0f);
    REQUIRE(ray.at(t) == expected);
}

TEST_CASE("RayDifferential default constructor", "[RayDifferential]") {
    jtx::RayDifferential ray;
    REQUIRE(ray.origin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.dir == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.time == 0.0f);
    REQUIRE(ray.rxOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.ryOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.rxDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.ryDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.hasDiffs == false);
}

TEST_CASE("RayDifferential constructor with parameters", "[RayDifferential]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 7.0f;
    jtx::RayDifferential ray(origin, dir, time);
    REQUIRE(ray.origin == origin);
    REQUIRE(ray.dir == dir);
    REQUIRE(ray.time == time);
    REQUIRE(ray.rxOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.ryOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.rxDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.ryDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(ray.hasDiffs == false);
}

TEST_CASE("RayDifferential copy constructor (RayDifferential)", "[RayDifferential]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 7.0f;
    jtx::RayDifferential ray(origin, dir, time);
    ray.rxOrigin = jtx::Point3f(1.0f, 1.0f, 1.0f);
    ray.ryOrigin = jtx::Point3f(2.0f, 2.0f, 2.0f);
    ray.rxDirection = jtx::Vec3f(3.0f, 3.0f, 3.0f);
    ray.ryDirection = jtx::Vec3f(4.0f, 4.0f, 4.0f);
    ray.hasDiffs = true;
    jtx::RayDifferential rayCopy(ray);
    REQUIRE(rayCopy.origin == origin);
    REQUIRE(rayCopy.dir == dir);
    REQUIRE(rayCopy.time == time);
    REQUIRE(rayCopy.rxOrigin == jtx::Point3f(1.0f, 1.0f, 1.0f));
    REQUIRE(rayCopy.ryOrigin == jtx::Point3f(2.0f, 2.0f, 2.0f));
    REQUIRE(rayCopy.rxDirection == jtx::Vec3f(3.0f, 3.0f, 3.0f));
    REQUIRE(rayCopy.ryDirection == jtx::Vec3f(4.0f, 4.0f, 4.0f));
    REQUIRE(rayCopy.hasDiffs == true);
}

TEST_CASE("RayDifferential copy constructor (Ray)", "[RayDifferential]") {
    jtx::Point3f origin(1.0f, 2.0f, 3.0f);
    jtx::Vec3f dir(4.0f, 5.0f, 6.0f);
    float time = 7.0f;
    jtx::Ray ray(origin, dir, time);
    jtx::RayDifferential rayCopy(ray);
    REQUIRE(rayCopy.origin == origin);
    REQUIRE(rayCopy.dir == dir);
    REQUIRE(rayCopy.time == time);
    REQUIRE(rayCopy.rxOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(rayCopy.ryOrigin == jtx::Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(rayCopy.rxDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(rayCopy.ryDirection == jtx::Vec3f(0.0f, 0.0f, 0.0f));
    REQUIRE(rayCopy.hasDiffs == false);
}

TEST_CASE("RayDifferential scale", "[RayDifferential]") {
    jtx::Point3f origin(0.0f, 0.0f, 0.0f);
    jtx::Vec3f dir(1.0f, 1.0f, 1.0f);
    jtx::RayDifferential ray(origin, dir, 0.0f);
    ray.rxOrigin = jtx::Point3f(1.0f, 1.0f, 1.0f);
    ray.ryOrigin = jtx::Point3f(2.0f, 2.0f, 2.0f);
    ray.rxDirection = jtx::Vec3f(1.0f, 1.0f, 1.0f);
    ray.ryDirection = jtx::Vec3f(2.0f, 2.0f, 2.0f);
    ray.hasDiffs = true;
    ray.scale(2.0f);
    REQUIRE(ray.rxOrigin == jtx::Point3f(2.0f, 2.0f, 2.0f));
    REQUIRE(ray.ryOrigin == jtx::Point3f(4.0f, 4.0f, 4.0f));
    REQUIRE(ray.rxDirection == jtx::Vec3f(1.0f, 1.0f, 1.0f));
    REQUIRE(ray.ryDirection == jtx::Vec3f(3.0f, 3.0f, 3.0f));
}

TEST_CASE("Ray valid", "[Ray]") {
    jtx::Ray ray;
    REQUIRE(ray.valid());
    ray.origin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    REQUIRE(ray.valid());
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    REQUIRE(ray.valid());
    ray.time = 7.0f;
    REQUIRE(ray.valid());
}

TEST_CASE("RayDifferential valid", "[RayDifferential]") {
    jtx::RayDifferential ray;
    REQUIRE(ray.valid());
    ray.origin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    REQUIRE(ray.valid());
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    REQUIRE(ray.valid());
    ray.time = 7.0f;
    REQUIRE(ray.valid());
    ray.rxOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    REQUIRE(ray.valid());
    ray.ryOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    REQUIRE(ray.valid());
    ray.rxDirection = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    REQUIRE(ray.valid());
    ray.ryDirection = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    REQUIRE(ray.valid());
    ray.hasDiffs = true;
    REQUIRE(ray.valid());
}

#ifdef JTX_TEST

TEST_CASE("Ray invalid", "[Ray]") {
    jtx::Ray ray;
    ray.origin = jtx::Point3f(std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f);
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.time = 7.0f;
    REQUIRE(!ray.valid());
    ray.origin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.dir = jtx::Vec3f(std::numeric_limits<float>::quiet_NaN(), 5.0f, 6.0f);
    REQUIRE(!ray.valid());
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.time = std::numeric_limits<float>::quiet_NaN();
    REQUIRE(!ray.valid());
}

TEST_CASE("RayDifferential invalid", "[RayDifferential]") {
    jtx::RayDifferential ray;
    ray.origin = jtx::Point3f(std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f);
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.time = 7.0f;
    ray.rxOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.ryOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.rxDirection = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.ryDirection = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.hasDiffs = true;
    REQUIRE(!ray.valid());
    ray.origin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.dir = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.time = 7.0f;
    ray.rxOrigin = jtx::Point3f(std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f);
    REQUIRE(!ray.valid());
    ray.rxOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.ryOrigin = jtx::Point3f(std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f);
    REQUIRE(!ray.valid());
    ray.ryOrigin = jtx::Point3f(1.0f, 2.0f, 3.0f);
    ray.rxDirection = jtx::Vec3f(std::numeric_limits<float>::quiet_NaN(), 5.0f, 6.0f);
    REQUIRE(!ray.valid());
    ray.rxDirection = jtx::Vec3f(4.0f, 5.0f, 6.0f);
    ray.ryDirection = jtx::Vec3f(std::numeric_limits<float>::quiet_NaN(), 5.0f, 6.0f);
    REQUIRE(!ray.valid());
}

#endif
