#pragma once

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <jtx.hpp>
#include <util/logger.hpp>

inline void checkVulkanError(const VkResult result, char const *const func, const char *const file, int const line) {
    if (result) {
        LOG_FATAL("Detected Vulkan error at {}:{} '{}': {}", file, line, func, string_VkResult(result));
    }
}

#define CHECK_VK(err) checkVulkanError((err), #err, __FILE__, __LINE__)

constexpr uint64_t JVK_TIMEOUT = 1000000000;
