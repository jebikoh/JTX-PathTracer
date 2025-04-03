#pragma once

#include "util/color.hpp"

struct Material {
    enum Type {
        DIFFUSE    = 0,
        DIELECTRIC = 1,
        CONDUCTOR  = 2,
    };

    Type mType = DIFFUSE;

    struct Parameters {
        Vec3 albedo;
        Vec3 ior;
        Vec3 k;
        Vec3 emission;
        float alphaY;
        float alphaX;
    } parameters;

    struct TextureIndices {
        uint albedo = -1;
    } textureIndices;
};

struct SurfaceIntersection {
    Vec3 point;
    Vec3 normal;
    Vec2f uv;
    Vec3 tangent;
    Vec3 bitangent;

    const Material *material;
    float t;
    bool frontFace;

    void setFaceNormal(const Ray &r, const Vec3 &n) {
        frontFace = jtx::dot(r.dir, n) < 0;
        normal    = frontFace ? n : -n;
    }
};
