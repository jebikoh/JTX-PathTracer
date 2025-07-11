#include <editor/gfx_context.hpp>
#include <engine/vulkan/accel.hpp>

namespace jtx {

inline bool HasFlag(const VkFlags item, const VkFlags flag) {
    return (item & flag) == flag;
}

void ASManager::BuildBLAS(const std::vector<BLASInput> &inputs, const VkBuildAccelerationStructureFlagsKHR flags) {
    const uint32_t numBLAS      = static_cast<uint32_t>(inputs.size());
    uint32_t numBLASCompactions = 0;

    VkDeviceSize totalSize{0};
    VkDeviceSize maxScratchSize{0};

    std::vector<AccelerationStructureBuildInfo> buildInfo(numBLAS);
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
    jvk::Buffer scratchBuffer = m_gfx.CreateBuffer(
        maxScratchSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        0,
        0,
        m_gfx.asProperties.minAccelerationStructureScratchOffsetAlignment);

    VkBufferDeviceAddressInfo bufferAddressInfo{};
    bufferAddressInfo.sType        = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferAddressInfo.buffer             = scratchBuffer.buffer;
    const VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_gfx.ctx, &bufferAddressInfo);

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
            m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](VkCommandBuffer cmd) {
                CreateBLAS(cmd, BLASIndices, buildInfo, scratchAddress, queryPool);
            });

            if (queryPool) {
                m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](VkCommandBuffer cmd) {
                    CompactBLAS(cmd, BLASIndices, buildInfo, queryPool);
                });
                DestroyNonCompactedBLAS(BLASIndices, buildInfo);
            }

            batchSize = 0;
            BLASIndices.clear();
        }
    }

    // Store BLAS handles
    for (const auto &info: buildInfo) {
        m_blas.push_back(info.as);
    }

    // Clean up
    vkDestroyQueryPool(m_gfx.ctx, queryPool, nullptr);
    m_gfx.DestroyBuffer(scratchBuffer);
}

void ASManager::BuildTLAS(const std::vector<VkAccelerationStructureInstanceKHR> &instances, const VkBuildAccelerationStructureFlagsKHR flags, const bool bUpdate) {
    assert(m_tlas.handle == VK_NULL_HANDLE || bUpdate);
    const uint32_t numInstances = static_cast<uint32_t>(instances.size());

    jvk::Buffer scratchBuffer;

    // Copy instance data to a staging buffer
    const auto bufSize              = numInstances * sizeof(VkAccelerationStructureInstanceKHR);
    jvk::Buffer stagingBuffer = m_gfx.CreateBuffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    void *data = stagingBuffer.Map(m_gfx.allocator);
    std::memcpy(data, instances.data(), bufSize);
    stagingBuffer.Unmap(m_gfx.allocator);

    jvk::Buffer instancesBuffer = m_gfx.CreateBuffer(
            numInstances * sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

    // Copy staging buffer to device buffer
    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size      = bufSize;
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, instancesBuffer.buffer, 1, &copyRegion);
    });

    m_gfx.DestroyBuffer(stagingBuffer);

    const VkDeviceAddress instancesAddress = instancesBuffer.GetDeviceAddress(m_gfx.ctx);

    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        VkMemoryBarrier barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        CreateTLAS(cmd, numInstances, instancesAddress, scratchBuffer, flags, bUpdate);
    });

    m_gfx.DestroyBuffer(scratchBuffer);
    m_gfx.DestroyBuffer(instancesBuffer);
}

void ASManager::DestroyAS() {
    for (auto &blas: m_blas) {
        vkDestroyAccelerationStructureKHR(m_gfx.ctx, blas.handle, nullptr);
        m_gfx.DestroyBuffer(blas.buffer);
    }
    m_blas.clear();

    if (m_tlas.handle != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(m_gfx.ctx, m_tlas.handle, nullptr);
        m_gfx.DestroyBuffer(m_tlas.buffer);
        m_tlas.handle = VK_NULL_HANDLE;
    }
}

AccelerationStructure ASManager::CreateAS(const VkAccelerationStructureCreateInfoKHR &info) const {
    AccelerationStructure as;

    // TODO: check VMA flags
    as.buffer = m_gfx.CreateBuffer(info.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    VkAccelerationStructureCreateInfoKHR createInfo = info;
    createInfo.buffer                               = as.buffer;
    vkCreateAccelerationStructureKHR(m_gfx.ctx, &createInfo, nullptr, &as.handle);

    if ((void *) vkGetAccelerationStructureDeviceAddressKHR != nullptr) {
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = as.handle;
        as.address                        = vkGetAccelerationStructureDeviceAddressKHR(m_gfx.ctx, &addressInfo);
    }

    return as;
}

void ASManager::CreateBLAS(const VkCommandBuffer cmd, const std::vector<uint32_t> &BLASIndices, std::vector<AccelerationStructureBuildInfo> &buildInfo, const VkDeviceAddress scratchAddress, const VkQueryPool queryPool) const {
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

void ASManager::CompactBLAS(const VkCommandBuffer cmd, const std::vector<uint32_t> &BLASIndices, std::vector<AccelerationStructureBuildInfo> &buildInfo, const VkQueryPool queryPool) const {
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

    for (auto &i: BLASIndices) {
        auto &info                          = buildInfo[i];
        info.cleanupAS                      = info.as;
        info.size.accelerationStructureSize = compactedSizes[queryCount++];

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.as          = CreateAS(createInfo);

        VkCopyAccelerationStructureInfoKHR copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
        copyInfo.src   = info.build.dstAccelerationStructure;
        copyInfo.dst   = info.as.handle;
        copyInfo.mode  = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);
    }
}

void ASManager::DestroyNonCompactedBLAS(const std::vector<uint32_t> &BLASIndices, std::vector<AccelerationStructureBuildInfo> &buildInfo) const {
    for (auto &i: BLASIndices) {
        vkDestroyAccelerationStructureKHR(m_gfx.ctx, buildInfo[i].cleanupAS.handle, nullptr);
        m_gfx.DestroyBuffer(buildInfo[i].cleanupAS.buffer);
    }
}

void ASManager::CreateTLAS(VkCommandBuffer cmd, uint32_t numInstances, VkDeviceAddress instancesAddress, jvk::Buffer &scratchBuffer, VkBuildAccelerationStructureFlagsKHR flags, bool bUpdate) {
    VkAccelerationStructureGeometryInstancesDataKHR instances{};
    instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instances.data.deviceAddress = instancesAddress;

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = instances;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.flags                    = flags;
    buildInfo.geometryCount            = 1;
    buildInfo.pGeometries              = &geometry;
    buildInfo.mode                     = bUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_gfx.ctx, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numInstances, &sizeInfo);

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.size  = sizeInfo.accelerationStructureSize;
    m_tlas           = CreateAS(createInfo);

    // TODO: check VMA flags
    scratchBuffer                  = m_gfx.CreateBuffer(
        sizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        0,
        0,
        m_gfx.asProperties.minAccelerationStructureScratchOffsetAlignment);
    VkDeviceAddress scratchAddress = scratchBuffer.GetDeviceAddress(m_gfx.ctx);

    buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure  = m_tlas.handle;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR offsetInfo{};
    offsetInfo.primitiveCount                                   = numInstances;
    const VkAccelerationStructureBuildRangeInfoKHR *pOffsetInfo = &offsetInfo;

    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pOffsetInfo);
}

}// namespace jtx