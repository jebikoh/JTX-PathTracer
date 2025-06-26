#pragma once

#include <jvk/jvk.hpp>
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

    void AddBinding(uint32_t binding, VkDescriptorType type);
    void AddBinding(uint32_t binding, uint32_t count, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, const void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {
    VkDescriptorPool pool;

    void InitPool(VkDevice device, uint32_t maxSets, std::span<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags = 0);
    void Reset(VkDevice device) const;
    void DestroyPool(VkDevice device) const;
    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout) const;
};

struct DynamicDescriptorAllocator {
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void Init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void ClearPools(VkDevice device);
    void DestroyPools(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, const void *pNext = nullptr);
private:
    VkDescriptorPool GetPool(VkDevice device);
    static VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool = 0;
};

struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> images;
    std::deque<std::vector<VkDescriptorImageInfo>> imageArrays;
    std::vector<VkDescriptorBufferInfo> buffers;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructures;

    std::vector<VkWriteDescriptorSet> writes;

    void WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteImages(int binding, std::span<VkDescriptorImageInfo> infos, VkDescriptorType type);

    void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    void WriteAS(int binding, const VkAccelerationStructureKHR *accelerationStructure);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);
};


}
