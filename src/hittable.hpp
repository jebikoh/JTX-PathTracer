#pragma once

#include "interval.hpp"
#include "rt.hpp"
#include <jtxlib/util/taggedptr.hpp>
#include "hit.hpp"
#include "material.hpp"

class Sphere;
class HittableList;

class Hittable : jtx::TaggedPtr<Sphere, HittableList> {
public:
    using TaggedPtr::TaggedPtr;

    bool hit(const Ray &r, Interval t, HitRecord &record) const;
};

class Sphere {
public:
    Sphere(const Vec3 &center, const Float radius, const Material &material)
        : _center(center, Vec3(0, 0, 0)),
          _radius(radius),
          _material(material) {}

    // Moving spheres
    Sphere(const Vec3 &start, const Vec3 &end, Float radius, const Material &material)
        : _center(start, end - start),
          _radius(radius),
          _material(material) {}

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        const auto currentCenter = _center.at(r.time);
        const Vec3 oc            = currentCenter - r.origin;
        const Float a            = r.dir.lenSqr();
        const Float h            = jtx::dot(r.dir, oc);
        const Float c            = oc.lenSqr() - _radius * _radius;

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
        const auto n = (record.point - currentCenter) / _radius;
        record.setFaceNormal(r, n);
        record.material = &_material;

        return true;
    }

private:
    Ray _center;
    Float _radius;
    Material _material;
};

class HittableList {
public:
    HittableList() = default;
    explicit HittableList(const Hittable &object) { add(object); }

    void add(const Hittable &object) { _objects.push_back(object); }
    void clear() { _objects.clear(); }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        HitRecord tmpRecord{};
        bool hitAnything  = false;
        auto closestSoFar = t.max;

        for (const auto &object: _objects) {
            if (object.hit(r, Interval(t.min, closestSoFar), tmpRecord)) {
                hitAnything  = true;
                closestSoFar = tmpRecord.t;
                record       = tmpRecord;
            }
        }

        return hitAnything;
    }

private:
    std::vector<Hittable> _objects;
};

inline bool Hittable::hit(const Ray &r, Interval t, HitRecord &record) const {
    auto fn = [&](auto ptr) { return ptr->hit(r, t, record); };
    return dispatch(fn);
}
