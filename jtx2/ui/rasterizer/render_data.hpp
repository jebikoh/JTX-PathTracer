#pragma once

#include "ui/jvk/buffer.hpp"
#include "ui/jvk/image.hpp"
#include "ui/jvk/jvk.hpp"
#include "ui/jvk/pipeline.hpp"
#include "ui/jvk/sampler.hpp"
#include "ui/jvk/descriptor.hpp"
#include <cstdint>

namespace jtx {

class Display;

/**
 * All meshes are stored in a single set of buffers, meaning, we can just
 * send the device addresses in the UBO instead of push constants.
 */
struct alignas(256) GPUDrawSceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 cameraPos;

    VkDeviceAddress vertexBufferAddress;
    VkDeviceAddress normalBufferAddress;
    VkDeviceAddress uvBufferAddress;
    VkDeviceAddress colorBufferAddress;
};

struct GPUDrawPushConstants {
    mat4 world;
    mat4 normal;
};

struct GPUMaterialInstance;

/**
 * A flattened struct containing all the data required to render a single object.
 */
struct GPURenderObject {
    uint32_t start;
    uint32_t count;
    VkBuffer indexBuffer;

    GPUMaterialInstance *material;

    mat4 transform;
    mat4 nTransform;
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

    jvk::Pipeline pipeline;
    VkDescriptorSet descriptorSet;
};

struct GPUMaterial {
    // Pipelines
    jvk::Pipeline opaquePipeline;
    jvk::Pipeline transparentPipeline;

    VkDescriptorSetLayout descriptorSetLayout;

    /**
     * Material data to be written to UBO
     */
    struct GPUMaterialData {
        // Temporary Blinn-Phong shading data
        vec4 ambient;
        vec4 diffuse;
        vec4 specular;
        float shininess;
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

    jvk::DescriptorWriter writer;

    void buildPipelines(Display *display);
    void destroy(VkDevice device) const;
    GPUMaterialInstance writeMaterial(VkDevice device, GPUMaterialPass pass, const GPUMaterialResources &resources, jvk::DynamicDescriptorAllocator &descriptorAllocator);
};
#pragma endregion

}