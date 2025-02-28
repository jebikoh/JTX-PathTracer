#pragma once

#include "util/color.hpp"

struct Material {
    enum Type {
        DIFFUSE = 0,
        DIELECTRIC = 1,
        CONDUCTOR = 2,
        METALLIC_ROUGHNESS = 3
    };

    Type type;
    Vec3 albedo;
    Float refractionIndex;
    Vec3 IOR;
    Vec3 k;
    float alphaX, alphaY;
    Vec3 emission = Vec3(0, 0, 0);
    
    int albedoTexId;
    int metallicRoughnessTexId;
};

struct SurfaceIntersection {
    Vec3 point;
    Vec3 normal;
    Vec2f uv;
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
