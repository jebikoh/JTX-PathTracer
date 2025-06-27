#pragma once
#include "jvk/commands.hpp"


#include <jvk/buffer.hpp>
#include <jvk/fence.hpp>
#include <jvk/jvk.hpp>

/**
 * This file contains all the logic related to creating and managing RT acceleration structures
 *
 * Currently EXTREMELY UNOPTIMIZED: there are numerous nested immediate buffer submissions, CPU stalls,
 * and no parallelization.
 */
namespace jtx {
struct GfxContext;

/**
 * Struct to hold required information from the user to build a BLAS
 */
struct BLASInput {
    std::vector<VkAccelerationStructureGeometryKHR> geometry;         // Geometry type, flags, triangle data references
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRange; // Indices within the vertex arrays for the BLAS
    VkBuildAccelerationStructureFlagsKHR flags = 0;                   // Build flags
};

/**
 * Represents a Vulkan acceleration structure (BLAS or TLAS)
 */
struct AccelerationStructure {
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    jvk::Buffer buffer;
    VkDeviceAddress address = 0;
};

/**
 * Information required to build an acceleration structure and the resulting acceleration structure
 */
struct AccelerationStructureBuildInfo {
    VkAccelerationStructureBuildGeometryInfoKHR build{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    const VkAccelerationStructureBuildRangeInfoKHR *pRange = nullptr;
    VkAccelerationStructureBuildSizesInfoKHR size{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    AccelerationStructure as{};        // Acceleration structure
    AccelerationStructure cleanupAS{}; // Acceleration structure to destroy after compaction
};

struct ASManager {
    explicit ASManager(const GfxContext &gfx)
        : m_gfx(gfx) {}

    /**
     * Builds a bottom-level acceleration structure (BLAS) for each input provided.
     * @param inputs vector of BLASInput structures for each BLAS to build
     * @param flags build flags to apply to each BLAS
     */
    void BuildBLAS(
        const std::vector<BLASInput> &inputs,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    /**
     * Builds a top-level acceleration structure (TLAS) from the provided instances.
     * @param instances vector of VkAccelerationStructureInstanceKHR structures representing instances
     * @param flags build flags to apply to the TLAS
     * @param bUpdate set to true if TLAS is being updated, false if it is being built from scratch
     */
    void BuildTLAS(
        const std::vector<VkAccelerationStructureInstanceKHR> &instances,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool bUpdate = false);

    /**
     * Retrieve the device address of a BLAS at the specified index.
     * @param index index of the BLAS
     * @return the device address of the BLAS
     */
    VkDeviceAddress GetBLASDeviceAddress(const size_t index) const {
        return m_blas[index].address;
    }

    VkAccelerationStructureKHR *GetTLAS() {
        return &m_tlas.handle;
    }

    /**
     * Destroys all acceleration structures and their associated buffers.
     */
    void DestroyAS();

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