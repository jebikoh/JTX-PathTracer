#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/vecmath.hpp>

using namespace jtx;

inline Vec3d randomInUnitSphere() {
    while (true) {
        Vec3 p = Vec3d::random(-1, 1);
        if (p.lenSqr() < 1) return p;
    }
}

inline Vec3d randomUnitVector() {
    return randomInUnitSphere().normalize();
};

inline Vec3d randomOnHemisphere(const Normal3d &normal) {
    Vec3d unit = randomUnitVector();
    if (dot(unit, normal) > 0.0) return unit;
    return -unit;
}

inline Vec3d randomInUnitDisk() {
    while (true) {
        auto p = Vec3d(random<double>(-1, 1), random<double>(-1, 1), 0);
        if (p.lenSqr() < 1) return p;
    }
}