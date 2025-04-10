#pragma once

#include "jvk.hpp"

namespace jvk {

struct Queue {
    VkQueue queue;
    uint32_t family;

    Queue() {};

    operator VkQueue() const { return queue; }

    VkResult submit(VkCommandBufferSubmitInfoKHR *cmdInfo, VkSemaphoreSubmitInfoKHR *waitSemaphoreInfo, VkSemaphoreSubmitInfoKHR *signalSemaphoreInfo, VkFence fence) const {
        VkSubmitInfo2KHR info            = {};
        info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext                    = nullptr;
        info.waitSemaphoreInfoCount   = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos      = waitSemaphoreInfo;
        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos    = signalSemaphoreInfo;
        info.commandBufferInfoCount   = 1;
        info.pCommandBufferInfos      = cmdInfo;
        return vkQueueSubmit2KHR(queue, 1, &info, fence);
    }
};

}// namespace jvk