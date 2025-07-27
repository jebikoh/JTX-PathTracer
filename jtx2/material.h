#pragma once
#include "jtx.hpp"

namespace jtx {

static constexpr int32_t JTX_MATERIAL_TEXTURE_INDEX_NONE = -1;
static constexpr int32_t JTX_MATERIAL_TEXTURE_MISSING    = -2;

struct Material {
    std::string name;

    enum Type {
        LAMBERTIAN        = 0,
        DIELECTRIC        = 1,
        COMPLEX_CONDUCTOR = 2,
        CONDUCTOR         = 3,
        THIN_DIELECTRIC   = 4,
        GLOSSY_DIFFUSE    = 5,
        OREN_NAYAR        = 6
    };

    Type mType = LAMBERTIAN;

    struct Parameters {
        vec3 diffuse;
        vec3 ior;
        vec3 k;
        vec3 f0;
        vec3 emission;
        float emissionStrength;
        float roughness;
        float anisotropy;
        float diffuseRoughness;
        bool bAnisotropic;
        float specularTint;
    } parameters;

    struct TextureIndices {
        int32_t baseColor         = JTX_MATERIAL_TEXTURE_INDEX_NONE;
    } textureIndices;

    bool IsEmissive() const { return parameters.emission.LengthSquared() > 0.0f && parameters.emissionStrength > 0.0f; }
};

}// namespace jtx
