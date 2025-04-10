#pragma once

#include "jvk.hpp"
#include <deque>

namespace jvk {

// VULKAN DESCRIPTORS
// VkDescriptorSetLayout -> VkDescriptorPool (Allocate) -> VkDescriptorSet
// VkDescriptorSet holds bindings (pointers) to various resources on GPU.
//
// Vulkan Pipelines have multiple slots for different descriptor sets.
// The specification guarantees at least 4 sets

// To create a descriptor layout, we need an array of bindings
struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void addBinding(uint32_t binding, uint32_t count, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, const void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct [[maybe_unused]] DescriptorAllocator {
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device) const;
    void destroyPool(VkDevice device) const;
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout) const;
};

struct DynamicDescriptorAllocator {
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, const void *pNext = nullptr);
private:
    VkDescriptorPool getPool(VkDevice device);
    static VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool = 0;
};

struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> images;
    std::deque<std::vector<VkDescriptorImageInfo>> imageArrays;

    std::vector<VkDescriptorBufferInfo> buffers;
    std::vector<VkWriteDescriptorSet> writes;

    void writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void writeImages(int binding, std::span<VkDescriptorImageInfo> infos, VkDescriptorType type);
    void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void clear();
    void updateSet(VkDevice device, VkDescriptorSet set);
};


}
