#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info{};

    bool IsValid() const { return buffer != VK_NULL_HANDLE; }

    operator VkBuffer() const { return buffer; }

    void Destroy(const VmaAllocator allocator) const {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    void *Map(const VmaAllocator allocator) const {
        void *data = nullptr;
        vmaMapMemory(allocator, allocation, &data);
        return data;
    }

    void Unmap(const VmaAllocator allocator) const {
        vmaUnmapMemory(allocator, allocation);
    }
};

}