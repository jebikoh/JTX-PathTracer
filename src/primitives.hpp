#pragma once

#include "util/aabb.hpp"
#include "material.hpp"

struct Primitive {
    enum Type {
        SPHERE = 0,
        TRIANGLE = 1,
    };

    Type type;
    size_t index;
    AABB bounds;

    [[nodiscard]]
    Vec3 centroid() const {
        return 0.5f * bounds.pmin + 0.5f * bounds.pmax;
    }
};

class Sphere {
public:
    Sphere(const Vec3 &center, const Float radius, Material *material)
        : center_(center, Vec3(0, 0, 0)),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
    }

    // Moving spheres
    Sphere(const Vec3 &start, const Vec3 &end, Float radius, Material *material)
        : center_(start, end - start),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
    }

    bool closestHit(const Ray &r, const Interval t, SurfaceIntersection &record) const {
        const auto currentCenter = center_.at(r.time);
        const Vec3 oc            = currentCenter - r.origin;
        const Float a            = r.dir.lenSqr();
        const Float h            = jtx::dot(r.dir, oc);
        const Float c            = oc.lenSqr() - radius_ * radius_;

        const Float discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }

        const auto sqrtd = jtx::sqrt(discriminant);
        auto root        = (h - sqrtd) / a;
        if (!t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!t.surrounds(root)) {
                return false;
            }
        }

        record.t     = root;
        record.point = r.at(root);
        const auto n = (record.point - currentCenter) / radius_;
        record.setFaceNormal(r, n);
        record.material = material_;

        return true;
    }

    bool anyHit(const Ray &r, const Interval t) const {
        const auto currentCenter = center_.at(r.time);
        const Vec3 oc            = currentCenter - r.origin;
        const Float a            = r.dir.lenSqr();
        const Float h            = jtx::dot(r.dir, oc);
        const Float c            = oc.lenSqr() - radius_ * radius_;

        const Float discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }

        const auto sqrtd = jtx::sqrt(discriminant);
        auto root        = (h - sqrtd) / a;
        if (!t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!t.surrounds(root)) {
                return false;
            }
        }
        return true;
    }

    AABB bounds() const {
        auto bbox = AABB(center_.at(0) - radius_, center_.at(0) + radius_);
        bbox.expand(AABB(center_.at(1) - radius_, center_.at(1) + radius_));
        return bbox;
    }

private:
    Ray center_;
    Float radius_;
    Material *material_;
};
