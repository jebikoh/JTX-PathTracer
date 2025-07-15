#pragma once

#include <VkBootstrap.h>
#include <jvk/context.hpp>
#include <jvk/jvk.hpp>

namespace jvk {

struct Swapchain {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format          = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> images{};
    std::vector<VkImageView> views{};
    VkExtent2D extent = {0, 0};

    Swapchain() = default;

    void Init(
            const VkContext &context,
            const uint32_t width,
            const uint32_t height,
            const VkFormat format_             = VK_FORMAT_R8G8B8A8_UNORM,
            const VkColorSpaceKHR colorSpace   = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR,
            const VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        vkb::SwapchainBuilder swapchainBuilder{context.physicalDevice, context.device, context.surface};
        vkb::Swapchain vkbSwapchain = swapchainBuilder
                                              .set_desired_format(
                                                      VkSurfaceFormatKHR{
                                                              .format     = format_,
                                                              .colorSpace = colorSpace})
                                              .set_desired_present_mode(presentMode)
                                              .set_desired_extent(width, height)
                                              .add_image_usage_flags(usageFlags)
                                              .build()
                                              .value();

        swapchain = vkbSwapchain.swapchain;
        format    = vkbSwapchain.image_format;
        images    = vkbSwapchain.get_images().value();
        views     = vkbSwapchain.get_image_views().value();
        extent    = vkbSwapchain.extent;
    }

    void Destroy(const VkContext &context) const {
        vkDestroySwapchainKHR(context, swapchain, nullptr);
        for (const auto imageView: views) {
            vkDestroyImageView(context, imageView, nullptr);
        }
    }

    VkResult AcquireNextImage(const VkContext &context, const VkSemaphore semaphore, uint32_t *imageIndex, const uint64_t timeout = JVK_TIMEOUT) const {
        return vkAcquireNextImageKHR(context, swapchain, timeout, semaphore, VK_NULL_HANDLE, imageIndex);
    }

    uint32_t GetSwapchainImageCount() const { return images.size(); }
};

}// namespace jvk