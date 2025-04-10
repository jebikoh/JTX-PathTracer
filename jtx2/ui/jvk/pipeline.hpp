#pragma once

#include "jvk.hpp"

namespace jvk {

bool loadShaderModule(const char *filePath, VkDevice device, VkShaderModule *outShaderModule);

struct PipelineBuilder {
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;// Shader modules for different stages
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;     // Triangle topology
    VkPipelineRasterizationStateCreateInfo _rasterizer;        // Rasterization settings between vertex & frag shader
    VkPipelineColorBlendAttachmentState _colorBlendAttachment; // Color blending & attachment information (transparency)
    VkPipelineMultisampleStateCreateInfo _multisampling;       // MSAA
    VkPipelineLayout _pipelineLayout;                          // Pipeline layout (descriptors, etc)
    VkPipelineDepthStencilStateCreateInfo _depthStencil;       // Depth-testing & stencil configuration
    VkPipelineRenderingCreateInfo _renderingInfo;              // Holds attachment info for pipeline, passed via pNext
    VkFormat _colorAttachmentFormat;

    // Pipeline parameters we don't configure:
    // - VkPipelineVertexInputStateCreateInfo: vertex attribute input configuration; we use "vertex pulling" so don't need it
    // - VkPipelineTesselationStateCreateInfo: fixed tesselation; we don't use it
    // - VkPipelineViewportStateCreateInfo:    information about rendering viewport; we are using dynamic state for this
    // - renderPass, subpass:                  we use dynamic rendering, so we just attach _renderingInfo into pNext.

    // We set up VkPipelineDynamicStateCreateInfo in the buildPipeline method for dynamic scissor and viewport

    PipelineBuilder() { clear(); }
    void clear();
    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);

    // Rasterizer state
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    // Multisampling
    void setMultiSamplingNone();
    void enableMultiSampling(VkSampleCountFlagBits sampleCount);
    void enableSampleShading(VkSampleCountFlagBits sampleCount, float minSampleShading);

    // Blending
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphaBlend();

    // Attachments
    void setColorAttachmentFormat(VkFormat format);
    void setDepthAttachmentFormat(VkFormat format);

    // Depth testing
    void disableDepthTest();
    void enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp);

    // Stencil
    void disableStencilTest();
    void enableStencilTest(const VkStencilOpState &front, const VkStencilOpState &back);

    VkPipeline buildPipeline(VkDevice device) const;
};

struct Pipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    void destroy(const VkDevice device, const bool destroyLayout = false) const {
        if (destroyLayout) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        vkDestroyPipeline(device, pipeline, nullptr);
    }
};

}// namespace VkUtil