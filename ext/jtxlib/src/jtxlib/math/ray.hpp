#pragma once

#include <jtxlib/math/vec3.hpp>
#include <jtxlib/math/vecmath.hpp>
#include <jtxlib/math/numerical.hpp>

namespace jtx {

    class Ray {
    public:
        Point3f origin;
        Vec3f dir;
        float time;
        // Add medium later

        [[nodiscard]] inline bool valid() const {
            return origin.valid() && dir.valid() && !jtx::isNaN(time);
        }

        //region Constructors
        Ray() : origin(), dir(), time(0.0f) {}

        Ray(const Point3f &origin, const Vec3f &direction, float time = 0.0f)
                : origin(origin), dir(direction), time(time) {
            ASSERT(valid());
        }

        Ray(const Ray &other) : origin(other.origin), dir(other.dir), time(other.time) {
            ASSERT(valid());
        }

        ~Ray() = default;
        //endregion

        [[nodiscard]] inline Point3f at(float t) const {
            return origin + t * dir;
        }
    };

    class RayDifferential : public Ray {
    public:
        Point3f rxOrigin, ryOrigin;
        Vec3f rxDirection, ryDirection;
        bool hasDiffs;

        // Intentionally hiding non-virtual function
        [[nodiscard]] inline bool valid() const {
            return Ray::valid() &&
                   (!hasDiffs || (rxOrigin.valid() && ryOrigin.valid() && rxDirection.valid() && ryDirection.valid()));
        }

        //region Constructors
        RayDifferential() : Ray(), rxOrigin(), ryOrigin(), rxDirection(), ryDirection(), hasDiffs(false) {}

        RayDifferential(const Point3f &origin, const Vec3f &direction, float time = 0.0f)
                : Ray(origin, direction, time), rxOrigin(), ryOrigin(), rxDirection(), ryDirection(), hasDiffs(false) {
            ASSERT(valid());
        }

        explicit RayDifferential(const Ray &ray) : Ray(ray), rxOrigin(), ryOrigin(), rxDirection(), ryDirection(),
                                                   hasDiffs(false) {
            ASSERT(valid());
        }

        RayDifferential(const RayDifferential &other) : Ray(other), rxOrigin(other.rxOrigin), ryOrigin(other.ryOrigin),
                                                        rxDirection(other.rxDirection), ryDirection(other.ryDirection),
                                                        hasDiffs(other.hasDiffs) {
            ASSERT(valid());
        }
        //endregion

        inline void scale(float s) {
            rxOrigin = origin + (rxOrigin - origin) * s;
            ryOrigin = origin + (ryOrigin - origin) * s;
            rxDirection = dir + (rxDirection - dir) * s;
            ryDirection = dir + (ryDirection - dir) * s;
        }
    };

} // jtx

