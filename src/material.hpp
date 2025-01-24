#pragma once

#include "util/color.hpp"

struct Material {
    enum Type {
        DIFFUSE,
        METAL,
        DIELECTRIC,
        DIFFUSE_LIGHT
    };

    Type type;
    Vec3 albedo;
    Float fuzz;
    Float refractionIndex;
    Vec3 emission = Color(0, 0, 0);
};

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    Vec3 tangent;
    Vec3 bitangent;

    const Material *material;
    Float t;
    bool frontFace;

    void setFaceNormal(const Ray &r, const Vec3 &n) {
        frontFace = jtx::dot(r.dir, n) < 0;
        normal    = frontFace ? n : -n;
    }
};
