#pragma once

#include "util/aabb.hpp"

struct Primitive {
    enum Type {
        SPHERE = 0,
        RECTANGLE = 1
    };

    Type type;
    int index;
};

class Sphere {
public:
    Sphere(const Vec3 &center, const Float radius, const Material &material)
        : center_(center, Vec3(0, 0, 0)),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
        bbox_ = AABB(center - r, center + r);
    }

    // Moving spheres
    Sphere(const Vec3 &start, const Vec3 &end, Float radius, const Material &material)
        : center_(start, end - start),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
        bbox_ = AABB(center_.at(0) - r, center_.at(0) + r);
        bbox_.expand(AABB(center_.at(1) - r, center_.at(1) + r));
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
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
        record.material = &material_;

        return true;
    }

    AABB bounds() const {
        return bbox_;
    }

private:
    Ray center_;
    Float radius_;
    const Material &material_;
    AABB bbox_;
};