#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

void TransitionImageIfNeeded(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

void CopyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
void CopyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, const VkExtent2D srcSize[2], const VkExtent2D dstSize[2]);

void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

// Taken from Sascha Willems repository:
// https://github.com/SaschaWillems/Vulkan/blob/e1c962289f33a87beff4f6d14e4c885483c3bd57/base/VulkanTools.cpp#L125
bool GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *pFormat);
bool GetSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat *pFormat);
bool FormatHasStencil(VkFormat format);
bool FormatHasDepth(VkFormat format);

struct ViewRectangle {
    int32_t x, y;
    uint32_t w, h;
};

}