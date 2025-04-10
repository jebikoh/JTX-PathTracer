#pragma once

#include "jvk.hpp"

namespace jvk {

struct Image {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;

    operator VkImage() { return image; }
    operator VkImageView() { return imageView; }

    void destroy(VkDevice device, VmaAllocator allocator) const {
        vkDestroyImageView(device, imageView, nullptr);
        vmaDestroyImage(allocator, image, allocation);
    }
};

}