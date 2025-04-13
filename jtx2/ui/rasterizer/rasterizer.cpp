#include "rasterizer.hpp"
#include "ui/display.hpp"
#include "scene/scene.hpp"

namespace jtx {

void Rasterizer::loadScene() {
    LOG_INFO(RASTERIZER, "Loading scene");
    if (m_bSceneLoaded) {
        clearGPUSceneBuffers();
    }

    const Scene *scene = m_pDisplay->m_pScene;

    // Calculate non-interleaved buffer sizes
    const size_t positionBufferSize = scene->positions.size() * sizeof(vec3);
    const size_t normalBufferSize   = scene->normals.size() * sizeof(vec3);
    const size_t uvBufferSize       = scene->uvs.size() * sizeof(vec2u);
    const size_t colorBufferSize    = scene->colors.size() * sizeof(vec3);
    const size_t indexBufferSize    = scene->indices.size() * sizeof(vec3u);
    const size_t totalSize          = positionBufferSize + normalBufferSize + uvBufferSize + colorBufferSize + indexBufferSize;

    bool bSceneHasVertexColors = colorBufferSize > 0;

    // Vertex buffers (position, normal, uv, color)
    constexpr VkBufferUsageFlags vertexBufferUsages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr VmaMemoryUsage bufferMemoryUsage      = VMA_MEMORY_USAGE_GPU_ONLY;

    LOG_DEBUG(RASTERIZER, "Creating GPU buffers");
    m_gpuSceneBuffers.position = m_pDisplay->createBuffer(positionBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(RASTERIZER, "Created position GPU buffer");
    m_gpuSceneBuffers.normal = m_pDisplay->createBuffer(normalBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(RASTERIZER, "Created normal GPU buffer");
    m_gpuSceneBuffers.uv = m_pDisplay->createBuffer(uvBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(RASTERIZER, "Created UV GPU buffer");
    if (bSceneHasVertexColors) {
        m_gpuSceneBuffers.color = m_pDisplay->createBuffer(colorBufferSize, vertexBufferUsages, bufferMemoryUsage);
        LOG_DEBUG(RASTERIZER, "Created color GPU buffer");
    }

    // Device addresses
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType           = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer          = m_gpuSceneBuffers.position.buffer;
    m_gpuSceneBuffers.positionAddress = vkGetBufferDeviceAddress(m_pDisplay->m_ctx, &deviceAddressInfo);
    deviceAddressInfo.buffer          = m_gpuSceneBuffers.normal.buffer;
    m_gpuSceneBuffers.normalAddress   = vkGetBufferDeviceAddress(m_pDisplay->m_ctx, &deviceAddressInfo);
    deviceAddressInfo.buffer          = m_gpuSceneBuffers.uv.buffer;
    m_gpuSceneBuffers.uvAddress       = vkGetBufferDeviceAddress(m_pDisplay->m_ctx, &deviceAddressInfo);
    if (bSceneHasVertexColors) {
        deviceAddressInfo.buffer       = m_gpuSceneBuffers.color.buffer;
        m_gpuSceneBuffers.colorAddress = vkGetBufferDeviceAddress(m_pDisplay->m_ctx, &deviceAddressInfo);
    }

    // Index buffer
    LOG_DEBUG(RASTERIZER, "Creating index buffer");
    constexpr VkBufferUsageFlags indexBufferUsages = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    m_gpuSceneBuffers.index                        = m_pDisplay->createBuffer(indexBufferSize, indexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(RASTERIZER, "Created index buffer");

    // Staging buffer
    LOG_DEBUG(RASTERIZER, "Creating staging buffer");
    jvk::Buffer staging = m_pDisplay->createBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(RASTERIZER, "Created staging buffer");

    void *data;
    vmaMapMemory(m_pDisplay->m_allocator, staging.allocation, &data);
    size_t offset = 0;
    memcpy(data, scene->indices.data(), indexBufferSize);
    offset += indexBufferSize;
    memcpy(static_cast<char *>(data) + offset, scene->positions.data(), positionBufferSize);
    offset += positionBufferSize;
    memcpy(static_cast<char *>(data) + offset, scene->normals.data(), normalBufferSize);
    offset += normalBufferSize;
    memcpy(static_cast<char *>(data) + offset, scene->uvs.data(), uvBufferSize);
    offset += uvBufferSize;
    if (bSceneHasVertexColors) {
        memcpy(static_cast<char *>(data) + offset, scene->colors.data(), colorBufferSize);
    }
    vmaUnmapMemory(m_pDisplay->m_allocator, staging.allocation);

    // Copy to GPU
    m_pDisplay->m_immBuffer.submit(m_pDisplay->m_graphicsQueue, [&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.dstOffset = 0;

        copyRegion.srcOffset = 0;
        copyRegion.size      = indexBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneBuffers.index.buffer, 1, &copyRegion);

        copyRegion.srcOffset += indexBufferSize;
        copyRegion.size = positionBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneBuffers.position.buffer, 1, &copyRegion);

        copyRegion.srcOffset += positionBufferSize;
        copyRegion.size = normalBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneBuffers.normal.buffer, 1, &copyRegion);

        copyRegion.srcOffset += normalBufferSize;
        copyRegion.size = uvBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneBuffers.uv.buffer, 1, &copyRegion);

        if (bSceneHasVertexColors) {
            copyRegion.srcOffset += uvBufferSize;
            copyRegion.size = colorBufferSize;
            vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneBuffers.color.buffer, 1, &copyRegion);
        }
    });

    m_pDisplay->destroyBuffer(staging);
    m_bSceneLoaded = true;
    LOG_INFO(RASTERIZER, "Scene loaded");
}

void Rasterizer::destroy() {
    clearGPUSceneBuffers();
}

void Rasterizer::clearGPUSceneBuffers() {
    if (m_bSceneLoaded) {
        LOG_DEBUG(RASTERIZER, "Destroying GPU scene buffers");
        LOG_DEBUG(RASTERIZER, "Destroying index buffer");
        m_pDisplay->destroyBuffer(m_gpuSceneBuffers.index);
        LOG_DEBUG(RASTERIZER, "Destroying vertex buffers");
        m_pDisplay->destroyBuffer(m_gpuSceneBuffers.position);
        LOG_DEBUG(RASTERIZER, "Destroying normal buffer");
        m_pDisplay->destroyBuffer(m_gpuSceneBuffers.normal);
        LOG_DEBUG(RASTERIZER, "Destroying UV buffer");
        m_pDisplay->destroyBuffer(m_gpuSceneBuffers.uv);
        if (!m_gpuSceneBuffers.color.isValid()) {
            LOG_DEBUG(RASTERIZER, "Destroying color buffer");
            m_pDisplay->destroyBuffer(m_gpuSceneBuffers.color);
        }
        m_bSceneLoaded = false;
        m_gpuSceneBuffers = {};
        LOG_DEBUG(RASTERIZER, "Destroyed GPU scene buffers");
    }
}

}// namespace jtx