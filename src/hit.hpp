#pragma once

#include "rt.hpp"

class Material;

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    const Material *material;
    Float t;
    bool frontFace;

    void setFaceNormal(const Ray &r, const Vec3 &n) {
        frontFace = jtx::dot(r.dir, n) < 0;
        normal    = frontFace ? n : -n;
    }
};