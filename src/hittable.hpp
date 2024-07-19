#pragma once
#include "rtx.hpp"

struct HitRecord {
    Point3d p;
    Vec3d normal;
    double t;
    bool frontFace;

    void setFaceNormal(const Rayd &r, const Vec3d &outwardNormal) {
        frontFace = dot(r.dir, outwardNormal) < 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Rayd &r, double tmin, double tmax, HitRecord &rec) const = 0;
    virtual bool hit(const Rayd &r, const Interval &t, HitRecord &rec) const = 0;
};
