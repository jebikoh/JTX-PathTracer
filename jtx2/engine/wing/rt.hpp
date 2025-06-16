#pragma once
#include <jvk/buffer.hpp>
#include <jvk/jvk.hpp>

/**
 * This file contains all the logic related to creating and managing RT acceleration structures
 *
 * Currently EXTREMELY UNOPTIMIZED: there are numerous nested immediate buffer submissions, CPU stalls,
 * and no parallelization.
 */
namespace jtx {
struct GfxContext;

struct BLASInput {
    std::vector<VkAccelerationStructureGeometryKHR> geometry;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRange;
    VkBuildAccelerationStructureFlagsKHR flags;
};

struct AccelerationStructure {
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    jvk::Buffer buffer;
    VkDeviceAddress address = 0;
};

struct AccelerationStructureBuildInfo {
    VkAccelerationStructureBuildGeometryInfoKHR build{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    const VkAccelerationStructureBuildRangeInfoKHR *pRange = nullptr;
    VkAccelerationStructureBuildSizesInfoKHR size{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    AccelerationStructure as{};
    AccelerationStructure cleanupAS{};
};

struct ASManager {
    explicit ASManager(const GfxContext &gfx)
        : m_gfx(gfx) {}

    void BuildBLAS(
        const std::vector<BLASInput> &inputs,
        VkBuildAccelerationStructureFlagsKHR flags);

    void BuildTLAS(
        const std::vector<VkAccelerationStructureInstanceKHR> &instances,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool bUpdate = false);

    VkDeviceAddress GetBLASDeviceAddress(const size_t index) const {
        return m_blas[index].address;
    }

    void Destroy();

private:
    const GfxContext &m_gfx;

    std::vector<AccelerationStructure> m_blas;
    AccelerationStructure m_tlas;

    AccelerationStructure CreateAS(const VkAccelerationStructureCreateInfoKHR &info) const;

    void CreateBLAS(
            VkCommandBuffer cmd,
            const std::vector<uint32_t> &BLASIndices,
            std::vector<AccelerationStructureBuildInfo> &buildInfo,
            VkDeviceAddress scratchAddress,
            VkQueryPool queryPool) const;
    void CompactBLAS(
            VkCommandBuffer cmd,
            const std::vector<uint32_t> &BLASIndices,
            std::vector<AccelerationStructureBuildInfo> &buildInfo,
            VkQueryPool queryPool) const;

    void DestroyNonCompactedBLAS(const std::vector<uint32_t> &BLASIndices, const std::vector<AccelerationStructureBuildInfo> &buildInfo) const;

    void CreateTLAS(
        VkCommandBuffer cmd,
        uint32_t numInstances,
        VkDeviceAddress instancesAddress,
        jvk::Buffer &scratchBuffer,
        VkBuildAccelerationStructureFlagsKHR flags,
        bool bUpdate);
};

}// namespace jtx