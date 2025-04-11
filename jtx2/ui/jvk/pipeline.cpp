#include "pipeline.hpp"
#include "init.hpp"
#include <fstream>

void jvk::PipelineBuilder::clear() {
    _inputAssembly        = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    _rasterizer           = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    _colorBlendAttachment = {};
    _multisampling        = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    _pipelineLayout       = {};
    _depthStencil         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    _renderingInfo        = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    _shaderStages.clear();
}

void jvk::PipelineBuilder::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
    _shaderStages.clear();
    _shaderStages.push_back(jvk::init::pipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    _shaderStages.push_back(jvk::init::pipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void jvk::PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
    _inputAssembly.topology               = topology;
    _inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void jvk::PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    _rasterizer.polygonMode = mode;
    _rasterizer.lineWidth   = 1.0f;
}

void jvk::PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    _rasterizer.cullMode  = cullMode;
    _rasterizer.frontFace = frontFace;
}

void jvk::PipelineBuilder::setMultiSamplingNone() {
    _multisampling.sampleShadingEnable   = VK_FALSE;
    _multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    _multisampling.minSampleShading      = 1.0f;
    _multisampling.pSampleMask           = nullptr;
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable      = VK_FALSE;
}

void jvk::PipelineBuilder::enableMultiSampling(VkSampleCountFlagBits sampleCount) {
    _multisampling.rasterizationSamples  = sampleCount;
    _multisampling.sampleShadingEnable   = VK_FALSE;
    _multisampling.minSampleShading      = 1.0f;
    _multisampling.pSampleMask           = nullptr;
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable      = VK_FALSE;
}

void jvk::PipelineBuilder::enableSampleShading(VkSampleCountFlagBits sampleCount, float minSampleShading) {
    _multisampling.rasterizationSamples  = sampleCount;
    _multisampling.sampleShadingEnable   = VK_TRUE;
    _multisampling.minSampleShading      = minSampleShading;
    _multisampling.pSampleMask           = nullptr;
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable      = VK_FALSE;
}


void jvk::PipelineBuilder::disableBlending() {
    _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable    = VK_FALSE;
}

void jvk::PipelineBuilder::setColorAttachmentFormat(VkFormat format) {
    _colorAttachmentFormat                 = format;
    _renderingInfo.colorAttachmentCount    = 1;
    _renderingInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void jvk::PipelineBuilder::setDepthAttachmentFormat(VkFormat format) {
    _renderingInfo.depthAttachmentFormat = format;
}

void jvk::PipelineBuilder::disableDepthTest() {
    _depthStencil.depthTestEnable       = VK_FALSE;
    _depthStencil.depthWriteEnable      = VK_FALSE;
    _depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
    _depthStencil.depthBoundsTestEnable = VK_FALSE;
    _depthStencil.stencilTestEnable     = VK_FALSE;
    _depthStencil.front                 = {};
    _depthStencil.back                  = {};
    _depthStencil.minDepthBounds        = 0.0f;
    _depthStencil.maxDepthBounds        = 1.0f;
}

void jvk::PipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp compareOp) {
    _depthStencil.depthTestEnable       = VK_TRUE;
    _depthStencil.depthWriteEnable      = depthWriteEnable;
    _depthStencil.depthCompareOp        = compareOp;
    _depthStencil.depthBoundsTestEnable = VK_FALSE;
    _depthStencil.stencilTestEnable     = VK_FALSE;
    _depthStencil.front                 = {};
    _depthStencil.back                  = {};
    _depthStencil.minDepthBounds        = 0.0f;
    _depthStencil.maxDepthBounds        = 1.0f;
}

void jvk::PipelineBuilder::disableStencilTest() {
    _depthStencil.stencilTestEnable = VK_FALSE;
    _depthStencil.front             = {};
    _depthStencil.back              = {};
}

void jvk::PipelineBuilder::enableStencilTest(const VkStencilOpState &front, const VkStencilOpState &back) {
    _depthStencil.stencilTestEnable = VK_TRUE;
    _depthStencil.back            = back;
    _depthStencil.front           = front;
}

VkPipeline jvk::PipelineBuilder::buildPipeline(const VkDevice device) const {
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext         = nullptr;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext           = nullptr;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &_colorBlendAttachment;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &_renderingInfo;
    pipelineInfo.stageCount          = static_cast<uint32_t>(_shaderStages.size());
    pipelineInfo.pStages             = _shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState   = &_multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &_depthStencil;
    pipelineInfo.layout              = _pipelineLayout;

    // DYNAMIC STATE
    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.pDynamicStates    = &state[0];
    dynamicInfo.dynamicStateCount = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        LOG_ERROR(VULKAN, "Failed to create pipeline");
        return VK_NULL_HANDLE;
    } else {
        return newPipeline;
    }
}

void jvk::PipelineBuilder::enableBlendingAdditive() {
    _colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable         = VK_TRUE;
    _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void jvk::PipelineBuilder::enableBlendingAlphaBlend() {
    _colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable         = VK_TRUE;
    _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    _colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}
