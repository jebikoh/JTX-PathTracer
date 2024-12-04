#pragma once

#include "rt.hpp"
#include <jtxlib/util/taggedptr.hpp>

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    Float t;
};

class Sphere {
public:
    Sphere(const Vec3 &center, Float radius) : _center(center), _radius(radius) {}

    bool hit(const Ray &r, Float tMin, Float tMax, HitRecord &record) const {
        const Vec3 oc = _center - r.origin;
        const Float a = r.dir.lenSqr();
        const Float h = jtx::dot(r.dir, oc);
        const Float c = oc.lenSqr() - _radius * _radius;

        const Float discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }

        auto sqrtd = jtx::sqrt(discriminant);
        auto root = (h - sqrtd) / a;
        if (root < tMin || root > tMax) {
            root = (h + sqrtd) / a;
            if (root < tMin || root > tMax) {
                return false;
            }
        }

        record.t = root;
        record.point = r.at(root);
        record.normal = (record.point - _center) / _radius;

        return true;
    }

private:
    Vec3  _center;
    Float _radius;
};

class Hittable : jtx::TaggedPtr<Sphere> {
public:
    using TaggedPtr::TaggedPtr;

    bool hit(const Ray &r, Float tMin, Float tMax, HitRecord &record) const {
        auto fn = [&](auto ptr) { return ptr->hit(r, tMin, tMax, record); };
        return dispatch(fn);
    }
};

