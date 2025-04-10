#include "descriptor.hpp"

namespace jvk {

void DescriptorLayoutBuilder::addBinding(const uint32_t binding, const VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding         = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType  = type;
    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::addBinding(const uint32_t binding, const uint32_t count, const VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding         = binding;
    newBinding.descriptorCount = count;
    newBinding.descriptorType  = type;
    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(const VkDevice device, const VkShaderStageFlags shaderStages, const void *pNext, const VkDescriptorSetLayoutCreateFlags flags) {
    for (auto &b: bindings) {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext        = pNext;
    info.pBindings    = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags        = flags;

    VkDescriptorSetLayout set;
    CHECK_VK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

void DescriptorAllocator::initPool(const VkDevice device, const uint32_t maxSets, const std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto [type, ratio]: poolRatios) {
        poolSizes.push_back({.type            = type,
                             .descriptorCount = static_cast<uint32_t>(ratio * maxSets)});
    }

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets       = maxSets;
    info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    info.pPoolSizes    = poolSizes.data();

    vkCreateDescriptorPool(device, &info, nullptr, &pool);
}

void DescriptorAllocator::clearDescriptors(const VkDevice device) const {
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroyPool(const VkDevice device) const {
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(const VkDevice device, const VkDescriptorSetLayout layout) const {
    VkDescriptorSetAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext              = nullptr;
    info.descriptorPool     = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts        = &layout;

    VkDescriptorSet ds;
    CHECK_VK(vkAllocateDescriptorSets(device, &info, &ds));

    return ds;
}

void DynamicDescriptorAllocator::init(const VkDevice device, const uint32_t initialSets, const std::span<PoolSizeRatio> poolRatios) {
    ratios.clear();
    for (auto r: poolRatios) {
        ratios.push_back(r);
    }

    const VkDescriptorPool newPool = createPool(device, initialSets, poolRatios);
    setsPerPool              = initialSets * 1.5;

    readyPools.push_back(newPool);
}

VkDescriptorPool DynamicDescriptorAllocator::getPool(const VkDevice device) {
    VkDescriptorPool pool;
    if (readyPools.size() != 0) {
        pool = readyPools.back();
        readyPools.pop_back();
    } else {
        pool = createPool(device, setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5;
        if (setsPerPool > 4092) {
            setsPerPool = 4092;
        }
    }

    return pool;
}

VkDescriptorPool DynamicDescriptorAllocator::createPool(const VkDevice device, const uint32_t setCount, const std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto [type, ratio]: poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
                .type            = type,
                .descriptorCount = static_cast<uint32_t>(ratio * setCount)});
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = 0;
    poolInfo.maxSets       = setCount;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
    return pool;
}

void DynamicDescriptorAllocator::clearPools(const VkDevice device) {
    for (auto p: readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p: fullPools) {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DynamicDescriptorAllocator::destroyPools(const VkDevice device) {
    for (auto p: readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (auto p: fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DynamicDescriptorAllocator::allocate(const VkDevice device, const VkDescriptorSetLayout layout, const void *pNext) {
    VkDescriptorPool pool = getPool(device);

    VkDescriptorSetAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext              = pNext;
    info.descriptorPool     = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts        = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &info, &ds);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        fullPools.push_back(pool);
        pool                = getPool(device);
        info.descriptorPool = pool;

        CHECK_VK(vkAllocateDescriptorSets(device, &info, &ds));
    }

    readyPools.push_back(pool);
    return ds;
}

void DescriptorWriter::writeImage(const int binding, const VkImageView image, const VkSampler sampler, const VkImageLayout layout, const VkDescriptorType type) {
    const VkDescriptorImageInfo &info = images.emplace_back(VkDescriptorImageInfo{
            .sampler     = sampler,
            .imageView   = image,
            .imageLayout = layout});

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding      = binding;
    write.dstSet          = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = &info;
    writes.push_back(write);
}

void DescriptorWriter::writeImages(const int binding, const std::span<VkDescriptorImageInfo> infos, const VkDescriptorType type) {
    // Deques ensure that emplace_back will maintain pointers
    imageArrays.emplace_back();
    auto &imageInfos = imageArrays.back();
    imageInfos.reserve(infos.size());
    for (const auto &info: infos) {
        imageInfos.push_back(info);
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;;
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    write.descriptorType = type;
    write.pImageInfo = imageInfos.data();
    writes.push_back(write);
}

void DescriptorWriter::writeBuffer(const int binding, const VkBuffer buffer, const size_t size, const size_t offset, const VkDescriptorType type) {
    const VkDescriptorBufferInfo &info = buffers.emplace_back(VkDescriptorBufferInfo{
            .buffer = buffer,
            .offset = offset,
            .range  = size});

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding      = binding;
    write.dstSet          = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = &info;
    writes.push_back(write);
}

void DescriptorWriter::clear() {
    images.clear();
    imageArrays.clear();
    buffers.clear();
    writes.clear();
}

void DescriptorWriter::updateSet(const VkDevice device, const VkDescriptorSet set) {
    for (VkWriteDescriptorSet &write: writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

}