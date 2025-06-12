#pragma once

#include <jvk/jvk.hpp>
#include <VkBootstrap.h>

namespace jvk {

struct VkContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkContext() = default;

    void Destroy() const {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
    }

    operator VkDevice() const { return device; } // NOLINT(*-explicit-constructor)
    operator VkInstance() const { return instance; } // NOLINT(*-explicit-constructor)
    operator VkPhysicalDevice() const { return physicalDevice; } // NOLINT(*-explicit-constructor)
    operator VkSurfaceKHR() const { return surface; } // NOLINT(*-explicit-constructor)
};

}