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
    VkDeviceAddress indexBuffer;
};

// Layout 1: Bindless scene resources
// Binding 0 (SSBO): GPUObjectData[]
struct GPUObjectData {
    glm::mat4 world;
    glm::mat4 normal;
    uint32_t start;
    uint32_t count;
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


/**
 * All meshes are stored in a single set of buffers, meaning, we can just
 * send the device addresses in the UBO instead of push constants.
 *
 */
struct alignas(256) GPUSceneUBOData {
    // TODO: change these to JTX mat4s
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 cameraPos;
    VkDeviceAddress vertexBufferAddress;
    VkDeviceAddress normalBufferAddress;
    VkDeviceAddress uvBufferAddress;
    VkDeviceAddress colorBufferAddress;
};

struct GPUDrawPushConstants {
    glm::mat4 world;
    glm::mat4 normal;
};

struct GPURayTracingPushConstants {
    glm::vec4 clearColor;
    glm::vec3 lightPosition;
    float lightIntensity;
    int lightType;
};

struct GPUMaterialInstance;

/**
 * A flattened struct containing all the data required to render a single object.
 */
struct GPURenderObject {
    uint32_t start;
    uint32_t count;

    GPUMaterialInstance *material;

    glm::mat4 transform;
    glm::mat4 nTransform;
};

struct GPUDrawContext {
    std::vector<GPURenderObject> opaque;
    std::vector<GPURenderObject> transparent;
};

#pragma region Material
enum class GPUMaterialPass : uint8_t {
    OPAQUE,
    TRANSPARENT
};

/**
 * Resources required for rendering a single material instance.
 */
struct GPUMaterialInstance {
    // We use this for draw sorting later
    GPUMaterialPass mType = GPUMaterialPass::OPAQUE;

    jvk::Pipeline *pipeline;
    VkDescriptorSet descriptorSet;
};

/**
 * GPU resources for the material instance.
 */
struct GPUMaterialResources {
    struct GPUMaterialImages {
        jvk::Image ambient;
        jvk::Image diffuse;
        jvk::Image specular;
    } images;

    struct GPUMaterialSamplers {
        jvk::Sampler ambient;
        jvk::Sampler diffuse;
        jvk::Sampler specular;
    } samplers;

    VkBuffer ubo;
    uint32_t uboOffset;
};

/**
 * Material data to be written to UBO
 */
struct alignas(256) GPUMaterialUBOData {
    // Temporary Blinn-Phong shading data
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};
#pragma endregion

#pragma region Other

struct GridPushConstants {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
};

#pragma endregion

}// namespace jtx