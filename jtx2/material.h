#pragma once
#include "jtx.hpp"

namespace jtx {

static constexpr int32_t JTX_MATERIAL_TEXTURE_INDEX_NONE = -1;
static constexpr int32_t JTX_MATERIAL_TEXTURE_MISSING    = -2;

struct Material {
    enum Type {
        DIFFUSE            = 0,
        DIELECTRIC         = 1,
        COMPLEX_CONDUCTOR  = 2,
        CONDUCTOR          = 3,
        METALLIC_ROUGHNESS = 4,
    };

    Type mType = DIFFUSE;

    struct Parameters {
        vec3 diffuse;
        vec3 ior;
        vec3 k;
        vec3 f0;
        vec3 emission;
        vec2 roughness;
    } parameters;

    struct TextureIndices {
        int32_t diffuse            = JTX_MATERIAL_TEXTURE_INDEX_NONE;
        int32_t metallicRoughness  = JTX_MATERIAL_TEXTURE_INDEX_NONE;
    } textureIndices;

    bool IsEmissive() const { return parameters.emission.LengthSquared() > 0.0f; }
};

}
