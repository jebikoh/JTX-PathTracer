#pragma once

#include <glm/mat4x4.hpp>
#include <jvk/buffer.hpp>
#include <jvk/image.hpp>
#include <jvk/jvk.hpp>
#include <jvk/pipeline.hpp>
#include <jvk/sampler.hpp>

namespace jtx {

class Display;

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
    glm::vec4 cameraPosition;
    glm::vec3 sunDirection;
    float sunIntensity;

    VkDeviceAddress vertexBuffer;
    VkDeviceAddress normalBuffer;
    VkDeviceAddress uvBuffer;
    VkDeviceAddress colorBuffer;
};

// Layout 1: Bindless scene resources
// Binding 0 (SSBO): GPUObjectData[]
struct GPUObjectData {
    glm::mat4 world;
    glm::mat4 normal;
    ResourceHandle material;
};

// Binding 1 (SSBO): GPUMaterialData[]
struct GPUMaterialData {
    glm::vec4 diffuse;
    glm::vec4 ior;
    glm::vec4 k;
    glm::vec4 f0;
    glm::vec4 emission;
    glm::vec4 roughness;
    TextureHandle diffuseTexture;
};

// Binding 2 (Combined Image Samplers): scene texture/sampler array

// Binding 3 (Acceleration Structure): TLAS if RT is supported


// == Push constants ==
struct GPUDrawPushConstants {
    ResourceHandle objectID;
};

struct GridPushConstants {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
};

struct GPURayTracingPushConstants {
    glm::vec4 clearColor;
    glm::vec3 lightPosition;
    float lightIntensity;
    int lightType;
};


// == Draw Context ==
/**
 * A flattened struct containing all the data required to render a single object.
 */
struct GPURenderObject {
    uint32_t start;
    uint32_t count;
    VkPipeline *materialPipeline;
};

struct GPUDrawContext {
    std::vector<GPURenderObject> objects;
};

}// namespace jtx