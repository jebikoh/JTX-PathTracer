#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

struct Fence {
    VkFence fence = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    Fence() = default;

    VkResult Init(VkDevice device_, VkFenceCreateFlags flags = 0) {
        device                 = device_;
        VkFenceCreateInfo info = {};
        info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext             = nullptr;
        info.flags             = flags;
        return vkCreateFence(device_, &info, nullptr, &fence);
    }

    operator VkFence() const { return fence; }

    VkResult Reset() const {
        return vkResetFences(device, 1, &fence);
    }

    VkResult Wait(const uint64_t timeout = JVK_TIMEOUT) const {
        return vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
    }

    void Destroy() const {
        vkDestroyFence(device, fence, nullptr);
    }
};

}// namespace jvk