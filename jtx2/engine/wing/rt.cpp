#include <engine/wing/rt.hpp>
#include <interface/gfx_context.hpp>

namespace jtx {

inline bool HasFlag(const VkFlags item, const VkFlags flag) {
    return (item & flag) == flag;
}

void ASBuilder::BuildBLAS(const std::vector<BLASInput> &inputs, const VkBuildAccelerationStructureFlagsKHR flags) {
    const uint32_t numBLAS      = static_cast<uint32_t>(inputs.size());
    uint32_t numBLASCompactions = 0;

    VkDeviceSize totalSize{0};
    VkDeviceSize maxScratchSize{0};

    std::vector<ASBuildInfo> buildInfo(numBLAS);
    for (size_t i = 0; i < numBLAS; ++i) {
        // VkAccelerationStructureBuildGeometryInfoKHR
        buildInfo[i].build.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo[i].build.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo[i].build.flags         = inputs[i].flags | flags;
        buildInfo[i].build.geometryCount = static_cast<uint32_t>(inputs[i].geometry.size());
        buildInfo[i].build.pGeometries   = inputs[i].geometry.data();

        // VkAccelerationStructureBuildRangeInfoKHR *
        buildInfo[i].pRange = inputs[i].buildRange.data();

        // VkAccelerationStructureBuildSizesInfoKHR
        std::vector<uint32_t> maxPrimitiveCount(inputs[i].buildRange.size());
        for (size_t j = 0; j < inputs[i].buildRange.size(); ++j) {
            maxPrimitiveCount[j] = inputs[i].buildRange[j].primitiveCount;
        }
        vkGetAccelerationStructureBuildSizesKHR(m_gfx.ctx, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo[i].build, maxPrimitiveCount.data(), &buildInfo[i].size);

        // Bookkeeping
        totalSize += buildInfo[i].size.accelerationStructureSize;
        maxScratchSize = std::max(maxScratchSize, buildInfo[i].size.buildScratchSize);
        numBLASCompactions += HasFlag(buildInfo[i].build.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
    }

    // Allocate scratch buffer
    // TODO: double check VMA flags
    const jvk::Buffer scratchBuffer = m_gfx.CreateBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo bufferAddressInfo{};
    bufferAddressInfo.sType        = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferAddressInfo.buffer       = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_gfx.ctx, &bufferAddressInfo);

    // Setup query pool to store compaction sizes
    VkQueryPool queryPool = VK_NULL_HANDLE;
    if (numBLASCompactions > 0) {
        assert(numBLASCompactions == numBLAS && "Compaction is only supported for all BLASes");
        VkQueryPoolCreateInfo info{};
        info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryCount = numBLASCompactions;
        info.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        vkCreateQueryPool(m_gfx.ctx, &info, nullptr, &queryPool);
    }

    std::vector<uint32_t> BLASIndices;
    VkDeviceSize batchSize            = 0;
    constexpr VkDeviceSize batchLimit = 256000000;// 256 MB

    for (uint32_t i = 0; i < numBLAS; ++i) {
        BLASIndices.push_back(i);
        batchSize += buildInfo[i].size.accelerationStructureSize;

        // Add BLAS to batch until size limit is reached, or we reach the last BLAS
        if (batchSize >= batchLimit || i == numBLAS - 1) {
            m_gfx.imBuffer.Submit(m_gfx.graphicsQueue, [&](VkCommandBuffer cmd) {
                CreateBLAS(cmd, BLASIndices, buildInfo, scratchAddress, queryPool);
            });

            if (queryPool) {
                m_gfx.imBuffer.Submit(m_gfx.graphicsQueue, [&](VkCommandBuffer cmd) {
                    CompactBLAS(cmd, BLASIndices, buildInfo, queryPool);
                });
                DestroyNonCompactedBLAS();
            }

            batchSize = 0;
            BLASIndices.clear();
        }
    }

    // Store build info for later use
    for (const auto &info: buildInfo) {
        m_blas.push_back(info);
    }

    // Clean up
    vkDestroyQueryPool(m_gfx.ctx, queryPool, nullptr);
    m_gfx.DestroyBuffer(scratchBuffer);
}

AccelerationStructure ASBuilder::CreateAS(const VkAccelerationStructureCreateInfoKHR &info) const {
    AccelerationStructure as;

    // TODO: check VMA flags
    as.buffer = m_gfx.CreateBuffer(info.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    VkAccelerationStructureCreateInfoKHR createInfo = info;
    createInfo.buffer = as.buffer;
    vkCreateAccelerationStructureKHR(m_gfx.ctx, &createInfo, nullptr, &as.handle);

    if ((void *) vkGetAccelerationStructureDeviceAddressKHR != nullptr) {
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = as.handle;
        VkDeviceAddress address = vkGetAccelerationStructureDeviceAddressKHR(m_gfx.ctx, &addressInfo);
    }

    return as;
}

void ASBuilder::CreateBLAS(const VkCommandBuffer cmd, const std::vector<uint32_t> &BLASIndices, std::vector<ASBuildInfo> &buildInfo, const VkDeviceAddress scratchAddress, const VkQueryPool queryPool) const {
    if (queryPool) vkResetQueryPool(m_gfx.ctx, queryPool, 0, static_cast<uint32_t>(BLASIndices.size()));
    uint32_t queryCount = 0;

    for (const auto &i: BLASIndices) {
        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.size  = buildInfo[i].size.accelerationStructureSize;
        createInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo[i].as  = CreateAS(createInfo);

        buildInfo[i].build.dstAccelerationStructure  = buildInfo[i].as.handle;
        buildInfo[i].build.scratchData.deviceAddress = scratchAddress;

        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo[i].build, &buildInfo[i].pRange);

        VkMemoryBarrier barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        if (queryPool) {
            vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, 1, &buildInfo[i].build.dstAccelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCount++);
        }
    }
}

void ASBuilder::CompactBLAS(const VkCommandBuffer cmd, const std::vector<uint32_t> &BLASIndices, std::vector<ASBuildInfo> &buildInfo, const VkQueryPool queryPool) const {
    uint32_t queryCount = 0;
    std::vector<AccelerationStructure> cleanupAS;

    // Get compacted BLAS sizes
    std::vector<VkDeviceSize> compactedSizes(BLASIndices.size());
    vkGetQueryPoolResults(
            m_gfx.ctx,
            queryPool,
            0,
            static_cast<uint32_t>(compactedSizes.size()),
            compactedSizes.size() * sizeof(VkDeviceSize),
            compactedSizes.data(),
            sizeof(VkDeviceSize),
            VK_QUERY_RESULT_WAIT_BIT);

    for (auto &info : buildInfo) {
        info.cleanupAS = info.as;
        info.size.accelerationStructureSize = compactedSizes[queryCount++];

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.as = CreateAS(createInfo);

        VkCopyAccelerationStructureInfoKHR copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
        copyInfo.src = info.build.dstAccelerationStructure;
        copyInfo.dst = info.as.handle;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);
    }
}

}// namespace jtx