#pragma once

#include <glm/mat4x4.hpp>
#include <jvk/buffer.hpp>
#include <jvk/image.hpp>
#include <jvk/jvk.hpp>
#include <jvk/pipeline.hpp>
#include <jvk/sampler.hpp>

namespace jtx {

class Editor;

using ResourceHandle = uint32_t;
using TextureHandle  = int32_t;

// == Descriptor Data ==
enum kL2Bindings {
    GPU_OBJECT_DATA           = 0,
    GPU_MATERIAL_DATA         = 1,
    GPU_TEXTURE_SAMPLER_ARRAY = 2,
    GPU_TLAS                  = 3,
};

// Layout 0: Per-frame global data
// Binding 0 (UBO):
struct alignas(256) GPUGlobalUniformData {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec3 cameraPosition;
    TextureHandle envmap;
    glm::vec3 skyColor;
    float skyIntensity;

    VkDeviceAddress vertexBuffer;
    VkDeviceAddress normalBuffer;
    VkDeviceAddress texCoordBuffer;
    VkDeviceAddress colorBuffer;
    VkDeviceAddress indexBuffer; // This is needed by RT
};

// Layout 1: Bindless scene resources
// Binding 0 (SSBO): GPUObjectData[]
struct GPUObjectData {
    glm::mat4 world;
    glm::mat4 normal;
    uint32_t startIndex;
    ResourceHandle material;

    uint32_t _padding[2];
};

// Binding 1 (SSBO): GPUMaterialData[]
struct GPUMaterialData {
    vec4 diffuse;
    vec4 ior;
    vec4 k;
    vec4 f0;
    vec4 roughness;
    vec4 emission;
    float emissionStrength;
    uint32_t type;

    TextureHandle diffuseTexture;
    TextureHandle _padding[1];
};

// Binding 2 (Combined Image Samplers): scene texture/sampler array

// Binding 3 (Acceleration Structure): TLAS if RT is supported


// == Push constants ==
struct DrawPushConstants {
    ResourceHandle objectID;
};

struct GridPushConstants {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
};

struct RayTracingPushConstants {
    glm::mat4 invProj;
    glm::mat4 invView;
    uint32_t frame;
    uint32_t samplesPerFrame;
};


// == Draw Context ==
/**
 * A flattened struct containing all the data required to render a single object.
 */
struct GPURenderObject {
    ResourceHandle objectID;
    uint32_t start;
    uint32_t count;
    VkPipeline *materialPipeline;
};

struct GPUDrawContext {
    std::vector<GPURenderObject> objects;
};

}// namespace jtx