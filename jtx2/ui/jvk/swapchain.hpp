#pragma once

#include <VkBootstrap.h>
#include "jvk.hpp"
#include "context.hpp"

namespace jvk {

struct Context;

struct Swapchain {
    VkSwapchainKHR swapchain;
    VkFormat imageFormat;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D extent;

    Swapchain() {};

    void init(
            Context &context,
            uint32_t width,
            uint32_t height,
            VkFormat format              = VK_FORMAT_B8G8R8A8_UNORM,
            VkColorSpaceKHR colorSpace   = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR,
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        vkb::SwapchainBuilder swapchainBuilder{context.physicalDevice, context.device, context.surface};
        vkb::Swapchain vkbSwapchain = swapchainBuilder
                                              .set_desired_format(
                                                      VkSurfaceFormatKHR{
                                                              .format     = format,
                                                              .colorSpace = colorSpace})
                                              .set_desired_present_mode(presentMode)
                                              .set_desired_extent(width, height)
                                              .add_image_usage_flags(usageFlags)
                                              .build()
                                              .value();

        swapchain   = vkbSwapchain.swapchain;
        imageFormat = format;
        images      = vkbSwapchain.get_images().value();
        imageViews  = vkbSwapchain.get_image_views().value();
        extent      = vkbSwapchain.extent;
    }

    void destroy(const Context &context) {
        vkDestroySwapchainKHR(context, swapchain, nullptr);
        for (int i = 0; i < imageViews.size(); ++i) {
            vkDestroyImageView(context, imageViews[i], nullptr);
        }
    }

    VkResult acquireNextImage(const Context &context, VkSemaphore semaphore, uint32_t *imageIndex, const uint64_t timeout = JVK_TIMEOUT) {
        return vkAcquireNextImageKHR(context, swapchain, timeout, semaphore, VK_NULL_HANDLE, imageIndex);
    }
};

}// namespace jvk