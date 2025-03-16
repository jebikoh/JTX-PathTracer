#pragma once

#include "util/color.hpp"

struct Material {
    enum Type {
        DIFFUSE = 0,
        DIELECTRIC = 1,
        CONDUCTOR = 2,
    };

    Type type = DIFFUSE;
    Vec3 albedo = Color::WHITE;
    Float refractionIndex = 1.5f;
    Vec3 IOR = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 k = Vec3(0.0f, 0.0f, 0.0f);
    float alphaX = 0;
    float alphaY = 0;
    Vec3 emission = Vec3(0, 0, 0);
    
    int albedoTexId = -1
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
