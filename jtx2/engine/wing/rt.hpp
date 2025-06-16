#pragma once
#include <jvk/buffer.hpp>
#include <jvk/jvk.hpp>

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

struct ASBuildInfo {
    VkAccelerationStructureBuildGeometryInfoKHR build{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    const VkAccelerationStructureBuildRangeInfoKHR *pRange = nullptr;
    VkAccelerationStructureBuildSizesInfoKHR size{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    AccelerationStructure as{};
    AccelerationStructure cleanupAS{};
};

struct ASBuilder {
    explicit ASBuilder(const GfxContext &gfx)
        : m_gfx(gfx) {}

    void BuildBLAS(const std::vector<BLASInput> &inputs, VkBuildAccelerationStructureFlagsKHR flags);

private:
    const GfxContext &m_gfx;
    std::vector<ASBuildInfo> m_blas;

    AccelerationStructure CreateAS(const VkAccelerationStructureCreateInfoKHR &info) const;

    void CreateBLAS(
            VkCommandBuffer cmd,
            const std::vector<uint32_t> &BLASIndices,
            std::vector<ASBuildInfo> &buildInfo,
            VkDeviceAddress scratchAddress,
            VkQueryPool queryPool) const;
    void CompactBLAS(
            VkCommandBuffer cmd,
            const std::vector<uint32_t> &BLASIndices,
            std::vector<ASBuildInfo> &buildInfo,
            VkQueryPool queryPool) const;
    void DestroyNonCompactedBLAS();
};

}// namespace jtx