#pragma once

#include <jvk/init.hpp>
#include <jvk/jvk.hpp>
#include <jvk/commands.hpp>
#include <jvk/fence.hpp>
#include <functional>

namespace jvk {

struct ImmediateBuffer {
    jvk::Fence fence;
    jvk::CommandPool pool;
    jvk::CommandBuffer cmd;

    ImmediateBuffer() = default;

    VkResult Init(const VkDevice device, const uint32_t familyIndex, const VkCommandPoolCreateFlagBits flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) {
        VkResult res = fence.Init(device);
        if (res != VK_SUCCESS) { return res; }
        res = pool.Init(device, familyIndex, flags);
        if (res != VK_SUCCESS) { return res; }
        return pool.AllocateCommandBuffer(&cmd);
    }

    void Destroy() const {
        pool.Destroy();
        fence.Destroy();
    }

    void Submit(const VkQueue queue, std::function<void(VkCommandBuffer cmd)> &&function) const {
        // Reset fence & buffer
        CHECK_VK(fence.Reset());
        CHECK_VK(cmd.Reset());

        // Create and start buffer
        CHECK_VK(cmd.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));

        // Record immediate submit commands
        function(cmd);

        // End buffer
        CHECK_VK(cmd.End());

        // Submit and wait for fence
        VkCommandBufferSubmitInfoKHR cmdInfo = cmd.SubmitInfo();
        const VkSubmitInfo2KHR submit        = jvk::init::Submit(&cmdInfo, nullptr, nullptr);
        CHECK_VK(vkQueueSubmit2KHR(queue, 1, &submit, fence));
        CHECK_VK(fence.Wait());
    }
};

}// namespace jvk
