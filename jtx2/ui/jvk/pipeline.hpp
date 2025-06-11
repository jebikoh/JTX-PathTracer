#pragma once

#include "jvk.hpp"

namespace jvk {
bool LoadShaderModule(const char *filePath, VkDevice device, VkShaderModule *outShaderModule);

struct PipelineBuilder {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};// Shader modules for different stages
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};     // Triangle topology
    VkPipelineRasterizationStateCreateInfo rasterization{};     // Rasterization settings between vertex & frag shader
    VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // Color blending & attachment information (transparency)
    VkPipelineMultisampleStateCreateInfo multisampling{};       // MSAA
    VkPipelineLayout pipelineLayout{};                          // Pipeline layout (descriptors, etc)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};       // Depth-testing & stencil configuration
    VkPipelineRenderingCreateInfo renderingInfo{};              // Holds attachment info for pipeline, passed via pNext
    VkFormat colorAttachmentFormat{};

    // Pipeline parameters we don't configure:
    // - VkPipelineVertexInputStateCreateInfo: vertex attribute input configuration; we use "vertex pulling" so don't need it
    // - VkPipelineTesselationStateCreateInfo: fixed tesselation; we don't use it
    // - VkPipelineViewportStateCreateInfo:    information about rendering viewport; we are using dynamic state for this
    // - renderPass, subpass:                  we use dynamic rendering, so we just attach _renderingInfo into pNext.

    // We set up VkPipelineDynamicStateCreateInfo in the buildPipeline method for dynamic scissor and viewport

    PipelineBuilder() { Clear(); }
    void Clear();
    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);

    // Rasterizer state
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    // Multisampling
    void SetMultiSamplingNone();
    void EnableMultiSampling(VkSampleCountFlagBits sampleCount);
    void EnableSampleShading(VkSampleCountFlagBits sampleCount, float minSampleShading);

    // Blending
    void DisableBlending();
    void EnableBlendingAdditive();
    void EnableBlendingAlphaBlend();

    // Attachments
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthAttachmentFormat(VkFormat format);

    // Depth testing
    void DisableDepthTest();
    void EnableDepthTest(bool depthWriteEnable, VkCompareOp compareOp);

    // Stencil
    void DisableStencilTest();
    void EnableStencilTest(const VkStencilOpState &front, const VkStencilOpState &back);

    VkPipeline BuildPipeline(VkDevice device) const;
};

struct Pipeline {
    VkPipeline pipeline     = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void Destroy(const VkDevice device, const bool destroyLayout = false) const {
        if (destroyLayout) {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
        vkDestroyPipeline(device, pipeline, nullptr);
    }
};

}// namespace jvk