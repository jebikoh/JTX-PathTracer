#include <jvk/descriptor.hpp>

namespace jvk {

void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding         = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType  = type;
    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const uint32_t count, const VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding         = binding;
    newBinding.descriptorCount = count;
    newBinding.descriptorType  = type;
    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::Clear() {
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(const VkDevice device, const VkShaderStageFlags shaderStages, const void *pNext, const VkDescriptorSetLayoutCreateFlags flags) {
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

void DescriptorAllocator::InitPool(const VkDevice device, const uint32_t maxSets, const std::span<VkDescriptorPoolSize> sizes, const VkDescriptorPoolCreateFlags flags) {
    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets       = maxSets;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes    = sizes.data();
    info.flags         = flags;

    vkCreateDescriptorPool(device, &info, nullptr, &pool);
}

void DescriptorAllocator::Reset(const VkDevice device) const {
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(const VkDevice device) const {
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(const VkDevice device, const VkDescriptorSetLayout layout) const {
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

void DynamicDescriptorAllocator::Init(const VkDevice device, const uint32_t initialSets, const std::span<PoolSizeRatio> poolRatios) {
    m_ratios.clear();
    for (auto r: poolRatios) {
        m_ratios.push_back(r);
    }

    const VkDescriptorPool newPool = CreatePool(device, initialSets, poolRatios);
    m_setsPerPool              = initialSets * 1.5;

    m_readyPools.push_back(newPool);
}

VkDescriptorPool DynamicDescriptorAllocator::GetPool(const VkDevice device) {
    VkDescriptorPool pool;
    if (m_readyPools.size() != 0) {
        pool = m_readyPools.back();
        m_readyPools.pop_back();
    } else {
        pool = CreatePool(device, m_setsPerPool, m_ratios);

        m_setsPerPool = m_setsPerPool * 1.5;
        if (m_setsPerPool > 4092) {
            m_setsPerPool = 4092;
        }
    }

    return pool;
}

VkDescriptorPool DynamicDescriptorAllocator::CreatePool(const VkDevice device, const uint32_t setCount, const std::span<PoolSizeRatio> poolRatios) {
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

void DynamicDescriptorAllocator::ClearPools(const VkDevice device) {
    for (auto p: m_readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p: m_fullPools) {
        vkResetDescriptorPool(device, p, 0);
        m_readyPools.push_back(p);
    }
    m_fullPools.clear();
}

void DynamicDescriptorAllocator::DestroyPools(const VkDevice device) {
    for (auto p: m_readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    m_readyPools.clear();
    for (auto p: m_fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    m_fullPools.clear();
}

VkDescriptorSet DynamicDescriptorAllocator::Allocate(const VkDevice device, const VkDescriptorSetLayout layout, const void *pNext) {
    VkDescriptorPool pool = GetPool(device);

    VkDescriptorSetAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext              = pNext;
    info.descriptorPool     = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts        = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &info, &ds);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        m_fullPools.push_back(pool);
        pool                = GetPool(device);
        info.descriptorPool = pool;

        CHECK_VK(vkAllocateDescriptorSets(device, &info, &ds));
    }

    m_readyPools.push_back(pool);
    return ds;
}


void DescriptorWriter::WriteImage(const int binding, const VkImageView image, const VkSampler sampler, const VkImageLayout layout, const VkDescriptorType type) {
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

void DescriptorWriter::WriteImage(const int binding, const int index, const VkImageView image, const VkSampler sampler, const VkImageLayout layout, const VkDescriptorType type) {
    const VkDescriptorImageInfo &info = images.emplace_back(VkDescriptorImageInfo{
            .sampler     = sampler,
            .imageView   = image,
            .imageLayout = layout});

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding      = binding;
    write.dstSet          = VK_NULL_HANDLE;
    write.dstArrayElement = index;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = &info;
    writes.push_back(write);
}

void DescriptorWriter::WriteImages(const int binding, const std::span<VkDescriptorImageInfo> infos, const VkDescriptorType type) {
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

void DescriptorWriter::WriteBuffer(const int binding, const VkBuffer buffer, const size_t size, const size_t offset, const VkDescriptorType type) {
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

void DescriptorWriter::WriteAS(const int binding, const VkAccelerationStructureKHR *accelerationStructure) {
    const VkWriteDescriptorSetAccelerationStructureKHR &info = accelerationStructures.emplace_back(VkWriteDescriptorSetAccelerationStructureKHR{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = accelerationStructure
    });

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pNext = &info;
    writes.push_back(write);
}

void DescriptorWriter::Clear() {
    images.clear();
    imageArrays.clear();
    buffers.clear();
    writes.clear();
}

void DescriptorWriter::UpdateSet(const VkDevice device, const VkDescriptorSet set) {
    for (VkWriteDescriptorSet &write: writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

}