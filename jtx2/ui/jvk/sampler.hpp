#pragma once

#include "jvk.hpp"

namespace jvk {

struct Sampler {
    VkSampler sampler;
    VkDevice device;

    Sampler() {};

    VkResult init(VkDevice device_, VkFilter minFilter, VkFilter magFilter) {
        device = device_;

        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.minFilter = minFilter;
        info.magFilter = magFilter;
        return vkCreateSampler(device, &info, nullptr, &sampler);
    }

    void destroy() const {
        vkDestroySampler(device, sampler, nullptr);
    }

    operator VkSampler() const { return sampler; }
};

}