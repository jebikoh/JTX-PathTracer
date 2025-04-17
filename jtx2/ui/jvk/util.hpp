#pragma once

#include "jvk.hpp"

namespace jvk {

void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

void copyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
void copyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize[2], VkExtent2D dstSize[2]);

void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

// Taken from Sascha Willems repository:
// https://github.com/SaschaWillems/Vulkan/blob/e1c962289f33a87beff4f6d14e4c885483c3bd57/base/VulkanTools.cpp#L125
bool getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *pFormat);
bool getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat *pFormat);
bool formatHasStencil(VkFormat format);
bool formatHasDepth(VkFormat format);

struct ViewRectangle {
  int32_t x, y;
  uint32_t w, h;
};

}