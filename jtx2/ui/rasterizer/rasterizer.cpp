#include "rasterizer.hpp"
#include "scene/scene.hpp"
#include "ui/display.hpp"
#include "ui/jvk/shaders.hpp"

namespace jtx {

void Rasterizer::init() {
    initSceneDataDescriptorSetLayout();
    initMaterialResources();
}

void Rasterizer::destroy() {
    destroyGPUSceneData();
    destroyMaterialResources();
    destroySceneDataDescriptorSetLayout();
}

void Rasterizer::loadScene() {
    LOG_INFO(RASTERIZER, "Loading scene");
    if (m_bSceneLoaded) {
        destroyGPUSceneData();
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
    LOG_DEBUG(RASTERIZER, "Vertex data copied to staging buffer");

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
    LOG_DEBUG(RASTERIZER, "Staging buffer copied to GPU");

    m_pDisplay->destroyBuffer(staging);

    // Load materials
    m_materialInstances.clear();
    m_materialInstances.reserve(scene->materials.size());

    LOG_DEBUG(RASTERIZER, "Creating material UBO");
    LOG_DEBUG(RASTERIZER, "Scene has {} materials", scene->materials.size());
    // Create UBO that can hold material data for all materials
    m_materialBufferUBO = m_pDisplay->createBuffer(sizeof(GPUMaterialDataUBO) * scene->materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(RASTERIZER, "Created material UBO");

    void *materialData;
    vmaMapMemory(m_pDisplay->m_allocator, m_materialBufferUBO.allocation, &materialData);

    size_t uboOffset = 0;
    for (const auto &material : scene->materials) {
        GPUMaterialDataUBO uboData{};
        uboData.diffuse = vec4(material.parameters.albedo, 1.0f);
        uboData.ambient = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        uboData.specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        uboData.shininess = 32.0f;
        static_cast<GPUMaterialDataUBO *>(materialData)[uboOffset] = uboData;

        GPUMaterialResources resources{};
        // For now, we are just going to use default textures/samplers
        resources.images.diffuse = m_pDisplay->m_whiteImage;
        resources.images.ambient = m_pDisplay->m_whiteImage;
        resources.images.specular = m_pDisplay->m_whiteImage;
        resources.samplers.diffuse = m_pDisplay->m_samplerLinear;
        resources.samplers.ambient = m_pDisplay->m_samplerLinear;
        resources.samplers.specular = m_pDisplay->m_samplerLinear;

        resources.ubo = m_materialBufferUBO;
        resources.uboOffset = uboOffset * sizeof(GPUMaterialDataUBO);

        GPUMaterialInstance mat = writeMaterial(GPUMaterialPass::OPAQUE, resources);
        m_materialInstances.push_back(mat);

        uboOffset++;
    }
    vmaUnmapMemory(m_pDisplay->m_allocator, m_materialBufferUBO.allocation);

    m_bSceneLoaded = true;
    LOG_INFO(RASTERIZER, "Scene loaded");
}

void Rasterizer::destroyGPUSceneData() {
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

        LOG_DEBUG(RASTERIZER, "Destroying material UBO");
        m_pDisplay->destroyBuffer(m_materialBufferUBO);

        m_bSceneLoaded    = false;
        m_gpuSceneBuffers = {};
        LOG_DEBUG(RASTERIZER, "Destroyed GPU scene buffers");
    }
}

void Rasterizer::initMaterialResources() {
    VkShaderModule vertShader;
    if (!jvk::loadShaderModule("../shaders/mesh.vert.spv", m_pDisplay->m_ctx, &vertShader)) {
        LOG_FATAL(RASTERIZER, "Failed to load vertex shader");
    }

    VkShaderModule fragShader;
    if (!jvk::loadShaderModule("../shaders/mesh.frag.spv", m_pDisplay->m_ctx, &fragShader)) {
        LOG_FATAL(RASTERIZER, "Failed to load fragment shader");
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.offset     = 0;
    pushConstantRange.size       = sizeof(GPUDrawPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    jvk::DescriptorLayoutBuilder builder;
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);        // Material UBO
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Ambient
    builder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Diffuse
    builder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Specular
    m_gpuMaterials.descriptorSetLayout = builder.build(m_pDisplay->m_ctx, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {m_sceneDataDescriptorSetLayout, m_gpuMaterials.descriptorSetLayout};

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::pipelineLayout();
    layoutInfo.setLayoutCount             = 2;
    layoutInfo.pSetLayouts                = layouts;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pushConstantRange;

    VkPipelineLayout layout;
    CHECK_VK(vkCreatePipelineLayout(m_pDisplay->m_ctx, &layoutInfo, nullptr, &layout));

    m_gpuMaterials.opaquePipeline.pipelineLayout      = layout;
    m_gpuMaterials.transparentPipeline.pipelineLayout = layout;

    jvk::PipelineBuilder pipelineBuilder;
    pipelineBuilder.setShaders(vertShader, fragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultiSamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder.disableStencilTest();
    pipelineBuilder.setColorAttachmentFormat(m_pDisplay->m_drawImage.image.imageFormat);
    pipelineBuilder.setDepthAttachmentFormat(m_pDisplay->m_drawImage.depthStencilImage.imageFormat);
    pipelineBuilder._pipelineLayout = layout;

    m_gpuMaterials.opaquePipeline.pipeline = pipelineBuilder.buildPipeline(m_pDisplay->m_ctx);

    pipelineBuilder.enableBlendingAdditive();
    m_gpuMaterials.transparentPipeline.pipeline = pipelineBuilder.buildPipeline(m_pDisplay->m_ctx);

    vkDestroyShaderModule(m_pDisplay->m_ctx, vertShader, nullptr);
    vkDestroyShaderModule(m_pDisplay->m_ctx, fragShader, nullptr);
}

void Rasterizer::destroyMaterialResources() const {
    vkDestroyDescriptorSetLayout(m_pDisplay->m_ctx, m_gpuMaterials.descriptorSetLayout, nullptr);
    m_gpuMaterials.opaquePipeline.destroy(m_pDisplay->m_ctx, true);
    m_gpuMaterials.transparentPipeline.destroy(m_pDisplay->m_ctx, false);
}

GPUMaterialInstance Rasterizer::writeMaterial(GPUMaterialPass pass, const GPUMaterialResources &resources) {
    GPUMaterialInstance mat{};
    mat.mType = pass;

    if (pass == GPUMaterialPass::OPAQUE) {
        mat.pipeline = m_gpuMaterials.opaquePipeline;
    } else if (pass == GPUMaterialPass::TRANSPARENT) {
        mat.pipeline = m_gpuMaterials.transparentPipeline;
    } else {
        LOG_ERROR(RASTERIZER, "Invalid material pass type");
        return {};
    }

    mat.descriptorSet = m_pDisplay->m_descriptorAllocator.allocate(m_pDisplay->m_ctx, m_gpuMaterials.descriptorSetLayout);

    writer.clear();
    writer.writeBuffer(0, resources.ubo, sizeof(GPUMaterialDataUBO), resources.uboOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(1, resources.images.ambient.imageView, resources.samplers.ambient, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(2, resources.images.diffuse.imageView, resources.samplers.diffuse, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(3, resources.images.specular.imageView, resources.samplers.specular, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(m_pDisplay->m_ctx, mat.descriptorSet);

    return mat;
}

void Rasterizer::populateContext() {
    m_drawContext.opaque.clear();
    m_drawContext.transparent.clear();

    // Loop through meshes
    for (const auto &mesh : m_pDisplay->m_pScene->meshes) {
        GPURenderObject obj{};
        obj.start = mesh.startIndex;
        obj.count = mesh.numIndices;
        obj.transform = mat4::identity();
        obj.nTransform = mat4::identity();
        // TODO
    }
}

void Rasterizer::initSceneDataDescriptorSetLayout() {
    jvk::DescriptorLayoutBuilder builder;
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    m_sceneDataDescriptorSetLayout = builder.build(m_pDisplay->m_ctx, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
}

void Rasterizer::destroySceneDataDescriptorSetLayout() const {
    vkDestroyDescriptorSetLayout(m_pDisplay->m_ctx, m_sceneDataDescriptorSetLayout, nullptr);
}

}// namespace jtx