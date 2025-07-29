#pragma once

#include <jvk/jvk.hpp>

namespace jvk {

struct Sampler {
    VkSampler sampler = VK_NULL_HANDLE;
    VkDevice device   = VK_NULL_HANDLE;

    Sampler() = default;

    VkResult Init(const VkDevice device_, const VkFilter minFilter, const VkFilter magFilter) {
        device = device_;

        VkSamplerCreateInfo info{};
        info.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.minFilter = minFilter;
        info.magFilter = magFilter;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        return vkCreateSampler(device, &info, nullptr, &sampler);
    }

    void Destroy() const {
        vkDestroySampler(device, sampler, nullptr);
    }

    operator VkSampler() const { return sampler; }
};

}