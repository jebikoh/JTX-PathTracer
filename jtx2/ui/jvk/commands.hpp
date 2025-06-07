#pragma once

#include "jvk.hpp"

namespace jvk {

struct CommandBuffer {
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    CommandBuffer() = default;
    explicit CommandBuffer(const VkCommandBuffer cmd_) : cmd(cmd_) {};

    operator VkCommandBuffer() const { return cmd; }

    VkResult Begin(const VkCommandBufferUsageFlags flags = 0) const {
        VkCommandBufferBeginInfo info = {};
        info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext                    = nullptr;
        info.pInheritanceInfo         = nullptr;
        info.flags                    = flags;
        return vkBeginCommandBuffer(cmd, &info);
    }

    VkResult End() const {
        return vkEndCommandBuffer(cmd);
    }

    VkResult Reset(const VkCommandBufferResetFlags flags = 0) const {
        return vkResetCommandBuffer(cmd, flags);
    }

    VkCommandBufferSubmitInfoKHR SubmitInfo() const {
        VkCommandBufferSubmitInfoKHR info = {};
        info.sType                     = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext                     = nullptr;
        info.commandBuffer             = cmd;
        info.deviceMask                = 0;
        return info;
    }
};

struct CommandPool {
    VkDevice device      = VK_NULL_HANDLE;
    VkCommandPool pool   = VK_NULL_HANDLE;
    uint32_t familyIndex = 0;

    CommandPool(CommandPool const &)            = delete;
    CommandPool &operator=(CommandPool const &) = delete;

    CommandPool() = default;

    VkResult Init(const VkDevice device_, const uint32_t familyIndex_, const VkCommandPoolCreateFlags flags) {
        device = device_;
        familyIndex = familyIndex_;

        VkCommandPoolCreateInfo info = {};
        info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext                   = nullptr;
        info.flags                   = flags;
        info.queueFamilyIndex        = familyIndex_;
        return vkCreateCommandPool(device_, &info, nullptr, &pool);
    }


    VkResult AllocateCommandBuffer(VkCommandBuffer *cmd, const uint32_t count = 1, const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        VkCommandBufferAllocateInfo info = {};
        info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext                       = nullptr;
        info.commandPool                 = pool;
        info.commandBufferCount          = count;
        info.level                       = level;
        return vkAllocateCommandBuffers(device, &info, cmd);
    }

    VkResult AllocateCommandBuffer(CommandBuffer *cmd, const uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        VkCommandBufferAllocateInfo info = {};
        info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext                       = nullptr;
        info.commandPool                 = pool;
        info.commandBufferCount          = count;
        info.level                       = level;
        return vkAllocateCommandBuffers(device, &info, &cmd->cmd);
    }

    operator VkCommandPool() const { return pool; }

    void Destroy() const {
        vkDestroyCommandPool(device, pool, nullptr);
    }
};

}// namespace jvk