#pragma once
#include "jvk.hpp"

namespace jvk::init {

inline VkCommandPoolCreateInfo commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
    VkCommandPoolCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext                   = nullptr;
    info.flags                   = flags;
    info.queueFamilyIndex        = queueFamilyIndex;
    return info;
}

inline VkCommandBufferAllocateInfo commandBuffer(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    VkCommandBufferAllocateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext                       = nullptr;
    info.commandPool                 = pool;
    info.commandBufferCount          = count;
    info.level                       = level;
    return info;
}

inline VkFenceCreateInfo fence(VkFenceCreateFlags flags = 0) {
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;
    info.flags             = flags;
    return info;
}

inline VkSemaphoreCreateInfo semaphore(VkSemaphoreCreateFlags flags = 0) {
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext                 = nullptr;
    info.flags                 = flags;
    return info;
}

inline VkCommandBufferBeginInfo commandBufferBegin(VkCommandBufferUsageFlags flags = 0) {
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext                    = nullptr;
    info.pInheritanceInfo         = nullptr;
    info.flags                    = flags;
    return info;
}

inline VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask) {
    VkImageSubresourceRange range = {};
    range.aspectMask              = aspectMask;
    range.baseMipLevel            = 0;
    range.levelCount              = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer          = 0;
    range.layerCount              = VK_REMAINING_ARRAY_LAYERS;
    return range;
}

inline VkSemaphoreSubmitInfoKHR semaphoreSubmit(VkPipelineStageFlags2KHR stageMask, VkSemaphore semaphore) {
    VkSemaphoreSubmitInfoKHR info = {};
    info.sType                    = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info.pNext                    = nullptr;
    info.semaphore                = semaphore;
    info.stageMask                = stageMask;
    info.deviceIndex              = 0;
    info.value                    = 1;
    return info;
}

inline VkCommandBufferSubmitInfoKHR commandBufferSubmit(VkCommandBuffer cmd) {
    VkCommandBufferSubmitInfoKHR info = {};
    info.sType                        = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext                        = nullptr;
    info.commandBuffer                = cmd;
    info.deviceMask                   = 0;
    return info;
}

inline VkSubmitInfo2KHR submit(VkCommandBufferSubmitInfoKHR *cmdInfo, VkSemaphoreSubmitInfoKHR *signalSemaphoreInfo, VkSemaphoreSubmitInfoKHR *waitSemaphoreInfo) {
    VkSubmitInfo2KHR info         = {};
    info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext                    = nullptr;
    info.waitSemaphoreInfoCount   = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos      = waitSemaphoreInfo;
    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos    = signalSemaphoreInfo;
    info.commandBufferInfoCount   = 1;
    info.pCommandBufferInfos      = cmdInfo;
    return info;
}

inline VkImageCreateInfo image(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) {
    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext             = nullptr;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = format;
    info.extent            = extent;
    info.mipLevels         = 1;
    info.arrayLayers       = 1;
    info.samples           = sampleCount;
    info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    info.usage             = usageFlags;
    return info;
}

inline VkImageViewCreateInfo imageView(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info           = {};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext                           = nullptr;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.image                           = image;
    info.format                          = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    info.subresourceRange.aspectMask     = aspectFlags;
    return info;
}

inline VkRenderingAttachmentInfo renderingAttachment(const VkImageView view, const VkClearValue *clear, const VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    VkRenderingAttachmentInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    info.pNext       = nullptr;
    info.imageView   = view;
    info.imageLayout = layout;
    info.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    info.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

    if (clear) {
        info.clearValue = *clear;
    }

    return info;
}

inline VkRenderingAttachmentInfo depthRenderingAttachment(const VkImageView view, const VkClearValue *clear, const VkImageLayout layout) {
    VkRenderingAttachmentInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    info.pNext       = nullptr;
    info.imageView   = view;
    info.imageLayout = layout;
    info.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    info.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear) {
        info.clearValue = *clear;
    }
    return info;
}

inline VkRenderingInfo rendering(const VkExtent2D renderExtent, const VkRenderingAttachmentInfo *colorAttachment, const VkRenderingAttachmentInfo *depthAttachment) {
    VkRenderingInfo info{};
    info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    info.pNext                = nullptr;
    info.renderArea           = VkRect2D{VkOffset2D{0, 0}, renderExtent};
    info.layerCount           = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments    = colorAttachment;
    info.pDepthAttachment     = depthAttachment;
    info.pStencilAttachment   = nullptr;
    return info;
}

inline VkPipelineShaderStageCreateInfo pipelineShaderStage(VkShaderStageFlagBits stage, VkShaderModule shader) {
    VkPipelineShaderStageCreateInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext  = nullptr;
    info.stage  = stage;
    info.module = shader;
    info.pName  = "main";
    return info;
}

inline VkPipelineLayoutCreateInfo pipelineLayout() {
    VkPipelineLayoutCreateInfo info{};
    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext                  = nullptr;
    info.flags                  = 0;
    info.setLayoutCount         = 0;
    info.pSetLayouts            = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges    = nullptr;
    return info;
}

inline VkPipelineLayoutCreateInfo pipelineLayout(VkDescriptorSetLayout *descriptorSetLayout, VkPushConstantRange *pushConstantRange) {
    VkPipelineLayoutCreateInfo info{};
    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext                  = nullptr;
    info.flags                  = 0;
    info.setLayoutCount         = 1;
    info.pSetLayouts            = descriptorSetLayout;
    info.pushConstantRangeCount = 1;
    info.pPushConstantRanges    = pushConstantRange;
    return info;
}

}// namespace jvk::init