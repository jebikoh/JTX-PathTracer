#pragma once

#include "jvk.hpp"

namespace jvk {

struct Fence {
    VkFence fence;
    VkDevice device;

    Fence() {};

    VkResult init(VkDevice device_, VkFenceCreateFlags flags = 0) {
        device                 = device_;
        VkFenceCreateInfo info = {};
        info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext             = nullptr;
        info.flags             = flags;
        return vkCreateFence(device_, &info, nullptr, &fence);
    }

    operator VkFence() const { return fence; }

    VkResult reset() const {
        return vkResetFences(device, 1, &fence);
    }

    VkResult wait(const uint64_t timeout = JVK_TIMEOUT) const {
        return vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
    }

    void destroy() {
        vkDestroyFence(device, fence, nullptr);
    }
};

}// namespace jvk