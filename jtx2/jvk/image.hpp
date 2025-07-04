#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D extent{};
    VkFormat format = VK_FORMAT_UNDEFINED;

    operator VkImage() const { return image; }
    operator VkImageView() const { return view; }

    void Destroy(const VkDevice device, const VmaAllocator allocator) const {
        vkDestroyImageView(device, view, nullptr);
        vmaDestroyImage(allocator, image, allocation);
    }
};

}