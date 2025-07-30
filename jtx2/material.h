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
        CONDUCTOR         = 2,
        THIN_DIELECTRIC   = 3,
        GLOSSY_DIFFUSE    = 4,
        OREN_NAYAR        = 5
    };

    Type mType = LAMBERTIAN;

    struct Parameters {
        vec3 diffuse{0.8f};
        vec3 ior{1.5f};
        vec3 f0{0.8f};
        vec3 emission{1.0f};
        vec3 transmissionColor{1.0f};
        float emissionStrength = 0.0f;
        float roughness        = 0.0f;
        float anisotropy       = 0.0f;
        float diffuseRoughness = 0.0f;
        bool bAnisotropic      = false;
        float specularTint     = 0.0f;
    } parameters;

    struct TextureIndices {
        int32_t baseColor         = JTX_MATERIAL_TEXTURE_INDEX_NONE;
    } textureIndices;

    bool IsEmissive() const { return parameters.emission.LengthSquared() > 0.0f && parameters.emissionStrength > 0.0f; }
};

}// namespace jtx
