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

    VkDeviceAddress GetDeviceAddress(const VkDevice device) const {
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = buffer;
        return vkGetBufferDeviceAddress(device, &info);
    }

    void *GetMapping() const {
        return info.pMappedData;
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