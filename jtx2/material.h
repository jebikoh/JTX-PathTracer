#pragma once
#include <jtx.hpp>

static constexpr int32_t JTX_MATERIAL_TEXTURE_INDEX_NONE = -1;

struct Material {
    enum Type {
        DIFFUSE            = 0,
        DIELECTRIC         = 1,
        CONDUCTOR          = 2,
        METALLIC_ROUGHNESS = 3,
    };

    Type mType = DIFFUSE;

    struct Parameters {
        vec3 albedo;
        vec3 ior;
        vec3 k;
        vec3 emission;

        float alphaX;
        float alphaY;
        float metallic;
        float roughness;
    } parameters;

    struct TextureIndices {
        int32_t albedo            = JTX_MATERIAL_TEXTURE_INDEX_NONE;
        int32_t metallicRoughness = JTX_MATERIAL_TEXTURE_INDEX_NONE;
    } textureIndices;

    bool isEmissive() const { return parameters.emission.lenSqr() > 0.0f; }
};