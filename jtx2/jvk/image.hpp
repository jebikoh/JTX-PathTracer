#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D imageExtent{};
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;

    operator VkImage() const { return image; }
    operator VkImageView() const { return imageView; }

    void Destroy(const VkDevice device, const VmaAllocator allocator) const {
        vkDestroyImageView(device, imageView, nullptr);
        vmaDestroyImage(allocator, image, allocation);
    }
};

}