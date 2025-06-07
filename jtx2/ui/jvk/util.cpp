#include "util.hpp"
#include "init.hpp"

namespace jvk {

void TransitionImageIfNeeded(const VkCommandBuffer cmd, const VkImage image, const VkImageLayout oldLayout, const VkImageLayout newLayout) {
    if (oldLayout == newLayout) {
        return; // No transition needed
    }
    TransitionImage(cmd, image, oldLayout, newLayout);
}

void TransitionImage(const VkCommandBuffer cmd, const VkImage image, const VkImageLayout oldLayout, const VkImageLayout newLayout) {
    // Creates a pipeline barrier stalls the pipeline until the image is ready
    // 1. All prior writes (srcAccessMask) from any stage (srcStageMask) must happen before the barrier
    // 2. The image layout (oldLayout) is transitioned to the new layout (newLayout)
    // 3. The image is now ready for reads and writes (dstAccessMask) from any stage (dstStageMask)

    VkImageMemoryBarrier2KHR imageBarrier = {};
    imageBarrier.sType                    = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    imageBarrier.pNext                    = nullptr;

    // Stage masks specify when the barrier is executed
    // Access masks specify what type of memory operations are being executed

    // We are stating that this image may be used in any pipeline stage
    // before and after this barrier.
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    // We are stating that before the barrier, we are writing to the image
    // and that after the barrier, we might read or write to the image.
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    // Specify the layout transition
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;

    // Target the correct part of the image
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    imageBarrier.subresourceRange = init::ImageSubresourceRange(aspectMask);
    imageBarrier.image            = image;

    // Create the dependency & submit
    VkDependencyInfoKHR depInfo     = {};
    depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext                   = nullptr;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &imageBarrier;
    vkCmdPipelineBarrier2KHR(cmd, &depInfo);
}

void CopyImageToImage(const VkCommandBuffer cmd, const VkImage src, const VkImage dst, const VkExtent2D srcSize, const VkExtent2D dstSize) {
    // Bit-block Transfer: copying data from one location to another
    // This is slower than `vkCmdCopyImage` but is more flexible
    VkImageBlit2KHR blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
    blitRegion.pNext = nullptr;

    blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcSize.width);
    blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcSize.height);
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstSize.width);
    blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstSize.height);
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    VkBlitImageInfo2KHR blitInfo = {};
    blitInfo.sType               = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR;
    blitInfo.pNext               = nullptr;
    blitInfo.srcImage            = src;
    blitInfo.srcImageLayout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage            = dst;
    blitInfo.dstImageLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.filter              = VK_FILTER_LINEAR;
    blitInfo.regionCount         = 1;
    blitInfo.pRegions            = &blitRegion;

    vkCmdBlitImage2KHR(cmd, &blitInfo);
}

void CopyImageToImage(const VkCommandBuffer cmd, const VkImage src, const VkImage dst, const VkExtent2D srcSize[2], const VkExtent2D dstSize[2]) {
    VkImageBlit2KHR blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
    blitRegion.pNext = nullptr;

    blitRegion.srcOffsets[0].x = static_cast<int32_t>(srcSize[0].width);
    blitRegion.srcOffsets[0].y = static_cast<int32_t>(srcSize[0].height);
    blitRegion.srcOffsets[0].z = 0;
    blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcSize[1].width);
    blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcSize[1].height);
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[0].x = static_cast<int32_t>(dstSize[0].width);
    blitRegion.dstOffsets[0].y = static_cast<int32_t>(dstSize[0].height);
    blitRegion.dstOffsets[0].z = 0;
    blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstSize[1].width);
    blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstSize[1].height);
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    VkBlitImageInfo2KHR blitInfo = {};
    blitInfo.sType               = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR;
    blitInfo.pNext               = nullptr;
    blitInfo.srcImage            = src;
    blitInfo.srcImageLayout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage            = dst;
    blitInfo.dstImageLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.filter              = VK_FILTER_LINEAR;
    blitInfo.regionCount         = 1;
    blitInfo.pRegions            = &blitRegion;

    vkCmdBlitImage2KHR(cmd, &blitInfo);
}

void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize) {
    const int mipLevels = static_cast<int>(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
    for (int mip = 0; mip < mipLevels; ++mip) {
        VkExtent2D halfSize = imageSize;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2KHR barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        barrier.pNext = nullptr;

        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange              = init::ImageSubresourceRange(aspectMask);
        barrier.subresourceRange.levelCount   = 1;
        barrier.subresourceRange.baseMipLevel = mip;
        barrier.image                         = image;

        VkDependencyInfoKHR dependencyInfo{};
        dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.pNext                   = nullptr;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);
        if (mip < mipLevels - 1) {
            VkImageBlit2KHR blit{};
            blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
            blit.pNext = nullptr;

            blit.srcOffsets[1].x = static_cast<int32_t>(imageSize.width);
            blit.srcOffsets[1].y = static_cast<int32_t>(imageSize.height);
            blit.srcOffsets[1].z = 1;

            blit.dstOffsets[1].x = static_cast<int32_t>(halfSize.width);
            blit.dstOffsets[1].y = static_cast<int32_t>(halfSize.height);
            blit.dstOffsets[1].z = 1;

            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;
            blit.srcSubresource.mipLevel       = mip;

            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;
            blit.dstSubresource.mipLevel       = mip + 1;

            VkBlitImageInfo2KHR blitInfo{};
            blitInfo.sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR;
            blitInfo.pNext          = nullptr;
            blitInfo.srcImage       = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.dstImage       = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.filter         = VK_FILTER_LINEAR;
            blitInfo.regionCount    = 1;
            blitInfo.pRegions       = &blit;

            vkCmdBlitImage2KHR(cmd, &blitInfo);
            imageSize = halfSize;
        }
    }

    TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

bool GetSupportedDepthFormat(const VkPhysicalDevice physicalDevice, VkFormat *pFormat) {
    const std::vector formats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM};

    for (auto &format: formats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            *pFormat = format;
            return true;
        }
    }

    return false;
}

bool GetSupportedDepthStencilFormat(const VkPhysicalDevice physicalDevice, VkFormat *pFormat) {
    const std::vector formats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT};

    for (auto &format: formats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            *pFormat = format;
            return true;
        }
    }

    return false;
}

bool FormatHasStencil(const VkFormat format) {
    std::vector formats = {
            VK_FORMAT_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    return std::ranges::find(formats, format) != formats.end();
}

bool FormatHasDepth(const VkFormat format) {
    std::vector formats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM};
    return std::ranges::find(formats, format) != formats.end();
}

}
