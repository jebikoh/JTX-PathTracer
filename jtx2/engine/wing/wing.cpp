#include <engine/wing/wing.hpp>
#include <interface/display.hpp>
#include <interface/ui_renderer.hpp>
#include <jvk/shaders.hpp>
#include <scene/scene.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <engine/wing/rt.hpp>

#include <glm/gtx/transform.hpp>

namespace jtx {

void WingEngine::Init(const bool bEnableRayTracing) {
    LOG_INFO(WING, "Initializing Rasterizer");
    m_bRayTracingAvailable = bEnableRayTracing;

    std::vector<jvk::DynamicDescriptorAllocator::PoolSizeRatio> sizes =
            {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};
    if (m_bRayTracingAvailable) {
        sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1});
    }
    m_descriptorAllocator.Init(m_gfx.ctx, 10, sizes);

    InitFrameSceneData();
    InitMaterialResources();
    InitGridPipeline();

    if (m_bRayTracingAvailable) {
        InitRayTracingDescriptors();
        InitRayTracingPipeline();
        InitRayTracingSBT();
    }

    LOG_INFO(WING, "Rasterizer initialized");
}

void WingEngine::Destroy() {
    LOG_INFO(WING, "Destroying Rasterizer");

    if (m_bRayTracingAvailable) {
        DestroyRayTracingSBT();
        DestroyRayTracingPipeline();
        DestroyRayTracingDescriptors();
    }

    DestroyGPUScene();
    DestroyGridPipeline();
    DestroyMaterialResources();
    DestroyFrameSceneData();
    m_descriptorAllocator.DestroyPools(m_gfx.ctx);

    LOG_INFO(WING, "Rasterizer destroyed");
}

void WingEngine::Draw(RenderContext &ctx, ResolveRegion &region) {
    if (!m_bSceneLoaded) {
        return;
    }

    UpdateSceneData();
    PopulateContext();
    // Calculate resolve region
    region.src[0].width = region.dst[0].width = m_viewRectangle.x;
    region.src[0].height = region.dst[0].height = m_viewRectangle.y;
    region.src[1].width = region.dst[1].width = m_viewRectangle.x + m_viewRectangle.w;
    region.src[1].height = region.dst[1].height = m_viewRectangle.y + m_viewRectangle.h;

    // Calculate viewport
    const VkRect2D renderArea{
                    {m_viewRectangle.x, m_viewRectangle.y},
                    {m_viewRectangle.w, m_viewRectangle.h}};

    if (m_bRayTracingEnabled) {
        RayTrace(ctx, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    } else {
        Rasterize(ctx, renderArea);
    }
}

void WingEngine::Rasterize(RenderContext &ctx, const VkRect2D &renderArea) {
    // Draw sorting
    std::vector<uint32_t> opaqueDraws;
    opaqueDraws.reserve(m_drawContext.opaque.size());
    for (uint32_t i = 0; i < m_drawContext.opaque.size(); i++) {
        opaqueDraws.push_back(i);
    }

    std::ranges::sort(opaqueDraws, [&](const auto &iA, const auto &iB) {
        const GPURenderObject &a = m_drawContext.opaque[iA];
        const GPURenderObject &b = m_drawContext.opaque[iB];
        if (a.material == b.material) {
            return a.start < b.start;
        }
        return a.material < b.material;
    });

    // Begin render pass
    VkClearValue drawImageClearValue{};
    drawImageClearValue.color = {0.255f, 0.247f, 0.255f, 1.0f};

    // Transition if needed
    jvk::TransitionImageIfNeeded(ctx.cmd, ctx.drawImage.image, ctx.layout.drawImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.layout.drawImage = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    jvk::TransitionImageIfNeeded(ctx.cmd, ctx.depthStencilImage.image, ctx.layout.depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    ctx.layout.depthStencilImage = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const VkRenderingAttachmentInfo colorAttachment = jvk::init::RenderingAttachment(ctx.drawImage.imageView, &drawImageClearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue dsClearValue{};
    dsClearValue.depthStencil.depth   = 1.0f;
    dsClearValue.depthStencil.stencil = 0;

    const VkRenderingAttachmentInfo depthAttachment = jvk::init::DepthRenderingAttachment(ctx.depthStencilImage.imageView, &dsClearValue, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo                   = jvk::init::Rendering(ctx.swapchain.extent, &colorAttachment, &depthAttachment);
    renderingInfo.renderArea                        = renderArea;

    vkCmdBeginRenderingKHR(ctx.cmd, &renderingInfo);

    // Update scene data UBO & descriptor set (layout 0, binding 0)
    const FrameData &frame = m_frameData[ctx.frameIndex];

    *frame.gpuSceneDataUBOMapping = m_gpuSceneUboData;

    const jvk::Pipeline *lastPipeline       = nullptr;
    const GPUMaterialInstance *lastMaterial = nullptr;

    // Bind index buffer
    vkCmdBindIndexBuffer(ctx.cmd, m_gpuSceneMeshData.index, 0, VK_INDEX_TYPE_UINT32);

    auto draw = [&](const GPURenderObject &r) {
        if (r.material != lastMaterial) {
            lastMaterial = r.material;

            if (r.material->pipeline != lastPipeline) {
                lastPipeline = r.material->pipeline;

                vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1, &frame.gpuSceneDataUboDescriptorSet, 0, nullptr);

                VkViewport viewport{};
                viewport.x        = static_cast<float>(m_viewRectangle.x);
                viewport.y        = static_cast<float>(m_viewRectangle.y);
                viewport.width    = static_cast<float>(m_viewRectangle.w);
                viewport.height   = static_cast<float>(m_viewRectangle.h);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(ctx.cmd, 0, 1, &viewport);

                // SCISSOR
                VkRect2D scissor{};
                scissor.offset.x      = m_viewRectangle.x;
                scissor.offset.y      = m_viewRectangle.y;
                scissor.extent.width  = m_viewRectangle.w;
                scissor.extent.height = m_viewRectangle.h;
                vkCmdSetScissor(ctx.cmd, 0, 1, &scissor);
            }

            vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1, &r.material->descriptorSet, 0, nullptr);
        }

        GPUDrawPushConstants pushConstants{};
        pushConstants.world  = glm::mat4(1.0f);
        pushConstants.normal = glm::mat4(1.0f);
        vkCmdPushConstants(ctx.cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

        // Need to multiply by 3 because r.count and r.start are relative to vec3u
        vkCmdDrawIndexed(ctx.cmd, r.count * 3, 1, r.start * 3, 0, 0);
    };

    for (const auto &r: opaqueDraws) {
        draw(m_drawContext.opaque[r]);
    }

    if (m_bDrawGrid) {
        vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gridPipeline.pipeline);

        GridPushConstants pushConstants{};
        pushConstants.viewProj    = m_gpuSceneUboData.viewProj;
        pushConstants.cameraPos   = m_gpuSceneUboData.cameraPos;
        pushConstants.invViewProj = glm::inverse(m_gpuSceneUboData.viewProj);
        vkCmdPushConstants(ctx.cmd, m_gridPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDrawIndexed(ctx.cmd, 3, 1, 0, 0, 0);
    }

    vkCmdEndRenderingKHR(ctx.cmd);
}

void WingEngine::ProcessEvent(const SDL_Event &event) {
    m_camera.processSDLEvent(event);
}

void WingEngine::SkipEvent() {
    m_camera.resetInputState();
}

void WingEngine::LoadScene(const Scene *pScene) {
    LOG_INFO(WING, "Loading scene");
    if (m_bSceneLoaded) {
        DestroyGPUScene();
    }
    m_scene = pScene;

    LOG_DEBUG(WING, "Loading textures");
    for (const auto &tex: pScene->textures) {
        auto format             = VK_FORMAT_R8G8B8A8_SRGB;
        const VkExtent3D extent = {static_cast<uint32_t>(tex.width), static_cast<uint32_t>(tex.height), 1};

        jvk::Image gpuTex;
        if (tex.channels < 4) {
            Image8u tex32b = tex.As32b();
            gpuTex         = m_gfx.CreateImage(tex32b.pData, extent, tex32b.channels, format, VK_IMAGE_USAGE_SAMPLED_BIT);
            tex32b.Destroy();
        } else {
            gpuTex = m_gfx.CreateImage(tex.pData, extent, tex.channels, format, VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        m_sceneTextures.push_back(gpuTex);
    }
    LOG_DEBUG(WING, "Textures loaded");

    // Calculate non-interleaved buffer sizes
    const size_t positionBufferSize = pScene->positions.size() * sizeof(vec3);
    const size_t normalBufferSize   = pScene->normals.size() * sizeof(vec3);
    const size_t uvBufferSize       = pScene->texCoords.size() * sizeof(vec2);
    const size_t colorBufferSize    = pScene->colors.size() * sizeof(vec3);
    const size_t indexBufferSize    = pScene->indices.size() * sizeof(vec3u);
    const size_t totalSize          = positionBufferSize + normalBufferSize + uvBufferSize + colorBufferSize + indexBufferSize;

    bool bSceneHasVertexColors = colorBufferSize > 0;

    // Vertex buffers (position, normal, uv, color)
    // TODO: AS build input should only be applied to index and vertex buffers
    VkBufferUsageFlags vertexBufferUsages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (m_bRayTracingAvailable) {
        vertexBufferUsages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    constexpr VmaMemoryUsage bufferMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    LOG_DEBUG(WING, "Creating GPU buffers");
    m_gpuSceneMeshData.position = m_gfx.CreateBuffer(positionBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(WING, "Created position GPU buffer");
    m_gpuSceneMeshData.normal = m_gfx.CreateBuffer(normalBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(WING, "Created normal GPU buffer");
    m_gpuSceneMeshData.uv = m_gfx.CreateBuffer(uvBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(WING, "Created UV GPU buffer");
    if (bSceneHasVertexColors) {
        m_gpuSceneMeshData.color = m_gfx.CreateBuffer(colorBufferSize, vertexBufferUsages, bufferMemoryUsage);
        LOG_DEBUG(WING, "Created color GPU buffer");
    }

    // Index buffer
    LOG_DEBUG(WING, "Creating index buffer");
    // We need VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT for ray tracing
    VkBufferUsageFlags indexBufferUsages = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (m_bRayTracingAvailable) {
        indexBufferUsages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    m_gpuSceneMeshData.index = m_gfx.CreateBuffer(indexBufferSize, indexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(WING, "Created index buffer");

    // Device addresses
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    m_gpuSceneMeshData.indexAddress    = m_gpuSceneMeshData.index.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneMeshData.positionAddress = m_gpuSceneMeshData.position.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneMeshData.normalAddress   = m_gpuSceneMeshData.normal.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneMeshData.uvAddress       = m_gpuSceneMeshData.uv.GetDeviceAddress(m_gfx.ctx);
    if (bSceneHasVertexColors) {
        m_gpuSceneMeshData.colorAddress = m_gpuSceneMeshData.color.GetDeviceAddress(m_gfx.ctx);
    }

    // Staging buffer
    LOG_DEBUG(WING, "Creating staging buffer");
    jvk::Buffer staging = m_gfx.CreateBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(WING, "Created staging buffer");

    void *data    = staging.Map(m_gfx.allocator);
    auto dataPtr  = static_cast<char *>(data);
    size_t offset = 0;

    std::ranges::copy(pScene->indices, reinterpret_cast<vec3u *>(dataPtr));
    offset += indexBufferSize;

    std::ranges::copy(pScene->positions, reinterpret_cast<vec3 *>(dataPtr + offset));
    offset += positionBufferSize;

    std::ranges::copy(pScene->normals, reinterpret_cast<vec3 *>(dataPtr + offset));
    offset += normalBufferSize;

    std::ranges::copy(pScene->texCoords, reinterpret_cast<vec2 *>(dataPtr + offset));
    offset += uvBufferSize;

    if (bSceneHasVertexColors) {
        std::ranges::copy(pScene->colors, reinterpret_cast<vec3 *>(dataPtr + offset));
    }

    staging.Unmap(m_gfx.allocator);
    LOG_DEBUG(WING, "Vertex data copied to staging buffer");

    // Copy to GPU
    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.dstOffset = 0;

        copyRegion.srcOffset = 0;
        copyRegion.size      = indexBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneMeshData.index.buffer, 1, &copyRegion);

        copyRegion.srcOffset += indexBufferSize;
        copyRegion.size = positionBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneMeshData.position.buffer, 1, &copyRegion);

        copyRegion.srcOffset += positionBufferSize;
        copyRegion.size = normalBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneMeshData.normal.buffer, 1, &copyRegion);

        copyRegion.srcOffset += normalBufferSize;
        copyRegion.size = uvBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneMeshData.uv.buffer, 1, &copyRegion);

        if (bSceneHasVertexColors) {
            copyRegion.srcOffset += uvBufferSize;
            copyRegion.size = colorBufferSize;
            vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneMeshData.color.buffer, 1, &copyRegion);
        }
    });
    LOG_DEBUG(WING, "Staging buffer copied to GPU");

    m_gfx.DestroyBuffer(staging);

    // Load materials
    m_gpuMaterialInstances.clear();
    m_gpuMaterialInstances.reserve(pScene->materials.size());

    LOG_DEBUG(WING, "Creating material UBO");
    LOG_DEBUG(WING, "Scene has {} materials", pScene->materials.size());
    // Create UBO that can hold material data for all materials
    m_materialBufferUBO = m_gfx.CreateBuffer(sizeof(GPUMaterialUBOData) * pScene->materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(WING, "Created material UBO");

    void *materialData = m_materialBufferUBO.Map(m_gfx.allocator);

    size_t uboOffset = 0;
    for (const auto &material: pScene->materials) {
        GPUMaterialUBOData uboData{};
        uboData.diffuse                                            = vec4(material.parameters.diffuse, 1.0f);
        uboData.ambient                                            = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        uboData.specular                                           = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        uboData.shininess                                          = 32.0f;
        static_cast<GPUMaterialUBOData *>(materialData)[uboOffset] = uboData;

        GPUMaterialResources resources{};
        if (material.textureIndices.diffuse != JTX_MATERIAL_TEXTURE_INDEX_NONE) {
            resources.images.diffuse = m_sceneTextures[material.textureIndices.diffuse];
        } else {
            resources.images.diffuse = m_gfx.defaultImages.white;
        }
        resources.images.ambient    = m_gfx.defaultImages.white;
        resources.images.specular   = m_gfx.defaultImages.white;
        resources.samplers.diffuse  = m_gfx.defaultSamplers.linear;
        resources.samplers.ambient  = m_gfx.defaultSamplers.linear;
        resources.samplers.specular = m_gfx.defaultSamplers.linear;

        resources.ubo       = m_materialBufferUBO;
        resources.uboOffset = uboOffset * sizeof(GPUMaterialUBOData);

        GPUMaterialInstance mat = WriteMaterial(GPUMaterialPass::OPAQUE, resources);
        m_gpuMaterialInstances.push_back(mat);

        uboOffset++;
    }
    m_materialBufferUBO.Unmap(m_gfx.allocator);

    m_bSceneLoaded = true;

    if (m_bRayTracingAvailable) {
        LOG_DEBUG(WING, "Initializing RT scene resources");
        BuildBLAS();
        BuildTLAS();

        jvk::DescriptorWriter writer;
        writer.WriteAS(0, m_ASManager.GetTLAS());
        writer.WriteImage(1, m_gfx.drawImage.image.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(m_gfx.ctx, m_rtDescriptorSet);

        LOG_DEBUG(WING, "RT scene resources initialized");
    }

    LOG_INFO(WING, "Scene loaded");
}

void WingEngine::DrawSettingsPanel(UiDrawContext &ctx) {
    ctx.StartRectangleBackground();
    if (ctx.StartTable("WingViewportTable")) {
        ctx.NewRow("Draw grid");
        ImGui::Checkbox("##Grid", &m_bDrawGrid);

        ctx.NewRow("Ray Tracing");
        ImGui::Checkbox("##RT", &m_bRayTracingEnabled);

        ctx.EndTable();
    }
    ctx.EndRectangleBackground();
}

void WingEngine::DestroyGPUSceneMeshData() {
    LOG_DEBUG(WING, "Destroying GPU scene buffers");
    LOG_DEBUG(WING, "Destroying index buffer");
    m_gfx.DestroyBuffer(m_gpuSceneMeshData.index);
    LOG_DEBUG(WING, "Destroying vertex buffers");
    m_gfx.DestroyBuffer(m_gpuSceneMeshData.position);
    LOG_DEBUG(WING, "Destroying normal buffer");
    m_gfx.DestroyBuffer(m_gpuSceneMeshData.normal);
    LOG_DEBUG(WING, "Destroying UV buffer");
    m_gfx.DestroyBuffer(m_gpuSceneMeshData.uv);
    if (m_gpuSceneMeshData.color.IsValid()) {
        LOG_DEBUG(WING, "Destroying color buffer");
        m_gfx.DestroyBuffer(m_gpuSceneMeshData.color);
    }

    m_gpuSceneMeshData = {};
    LOG_DEBUG(WING, "Destroyed GPU scene buffers");
}

void WingEngine::InitMaterialResources() {
    VkShaderModule vertShader;
    if (!jvk::LoadShaderModule("../shaders/mesh.vert.spv", m_gfx.ctx, &vertShader)) {
        LOG_FATAL(WING, "Failed to load vertex shader");
    }

    VkShaderModule fragShader;
    if (!jvk::LoadShaderModule("../shaders/mesh.frag.spv", m_gfx.ctx, &fragShader)) {
        LOG_FATAL(WING, "Failed to load fragment shader");
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.offset     = 0;
    pushConstantRange.size       = sizeof(GPUDrawPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    jvk::DescriptorLayoutBuilder builder;
    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);        // Material UBO
    builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Ambient
    builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Diffuse
    builder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Specular
    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (m_bRayTracingAvailable) {
        stages |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    }
    m_gpuMaterials.descriptorSetLayout = builder.Build(m_gfx.ctx, stages);

    VkDescriptorSetLayout layouts[] = {m_gpuSceneDataUboDescriptorLayout, m_gpuMaterials.descriptorSetLayout};

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::PipelineLayout();
    layoutInfo.setLayoutCount             = 2;
    layoutInfo.pSetLayouts                = layouts;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pushConstantRange;

    VkPipelineLayout layout;
    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &layout));

    m_gpuMaterials.opaquePipeline.layout      = layout;
    m_gpuMaterials.transparentPipeline.layout = layout;

    jvk::PipelineBuilder pipelineBuilder;
    pipelineBuilder.SetShaders(vertShader, fragShader);
    pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.SetMultiSamplingNone();
    pipelineBuilder.DisableBlending();
    pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder.DisableStencilTest();
    pipelineBuilder.SetColorAttachmentFormat(m_gfx.drawImage.image.imageFormat);
    pipelineBuilder.SetDepthAttachmentFormat(m_gfx.drawImage.depthStencilImage.imageFormat);
    pipelineBuilder.pipelineLayout = layout;

    m_gpuMaterials.opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(m_gfx.ctx);

    pipelineBuilder.EnableBlendingAdditive();
    m_gpuMaterials.transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(m_gfx.ctx);

    vkDestroyShaderModule(m_gfx.ctx, vertShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, fragShader, nullptr);
}

void WingEngine::DestroyMaterialResources() const {
    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_gpuMaterials.descriptorSetLayout, nullptr);
    m_gpuMaterials.opaquePipeline.Destroy(m_gfx.ctx, true);
    m_gpuMaterials.transparentPipeline.Destroy(m_gfx.ctx, false);
}

void WingEngine::DestroyGPUSceneMaterials() const {
    m_gfx.DestroyBuffer(m_materialBufferUBO);
}

void WingEngine::DestroyGPUSceneTextures() const {
    for (const auto &img: m_sceneTextures) {
        img.Destroy(m_gfx.ctx, m_gfx.allocator);
    }
}

GPUMaterialInstance WingEngine::WriteMaterial(const GPUMaterialPass pass, const GPUMaterialResources &resources) {
    GPUMaterialInstance mat{};
    mat.mType = pass;

    if (pass == GPUMaterialPass::OPAQUE) {
        mat.pipeline = &m_gpuMaterials.opaquePipeline;
    } else if (pass == GPUMaterialPass::TRANSPARENT) {
        mat.pipeline = &m_gpuMaterials.transparentPipeline;
    } else {
        LOG_ERROR(WING, "Invalid material pass type");
        return {};
    }

    mat.descriptorSet = m_descriptorAllocator.Allocate(m_gfx.ctx, m_gpuMaterials.descriptorSetLayout);

    m_descriptorWriter.Clear();
    m_descriptorWriter.WriteBuffer(0, resources.ubo, sizeof(GPUMaterialUBOData), resources.uboOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    m_descriptorWriter.WriteImage(1, resources.images.ambient.imageView, resources.samplers.ambient, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_descriptorWriter.WriteImage(2, resources.images.diffuse.imageView, resources.samplers.diffuse, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_descriptorWriter.WriteImage(3, resources.images.specular.imageView, resources.samplers.specular, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    m_descriptorWriter.UpdateSet(m_gfx.ctx, mat.descriptorSet);

    return mat;
}

void WingEngine::PopulateContext() {
    m_drawContext.opaque.clear();
    m_drawContext.transparent.clear();

    // Loop through meshes
    for (const auto &mesh: m_scene->meshes) {
        GPURenderObject obj{};
        obj.start      = mesh.startIndex;
        obj.count      = mesh.numIndices;
        obj.transform  = glm::mat4(1.0f);
        obj.nTransform = glm::mat4(1.0f);
        obj.material   = &m_gpuMaterialInstances[mesh.materialIndex];
        m_drawContext.opaque.push_back(obj);
    }
}

void WingEngine::UpdateSceneData() {
    m_camera.update();

    float aspectRatio;
    if (m_viewRectangle.w > 0 && m_viewRectangle.h > 0) {
        aspectRatio = static_cast<float>(m_viewRectangle.w) / static_cast<float>(m_viewRectangle.h);
    } else {
        aspectRatio = static_cast<float>(m_gfx.window.extent.width) / static_cast<float>(m_gfx.window.extent.width);
    }

    const glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj       = glm::perspective(glm::radians(70.f), aspectRatio, 0.1f, 10000.0f);
    proj[1][1] *= -1;

    m_gpuSceneUboData.view                = view;
    m_gpuSceneUboData.proj                = proj;
    m_gpuSceneUboData.viewProj            = proj * view;
    m_gpuSceneUboData.cameraPos           = glm::vec4(m_camera.position, 0.0f);
    m_gpuSceneUboData.vertexBufferAddress = m_gpuSceneMeshData.positionAddress;
    m_gpuSceneUboData.normalBufferAddress = m_gpuSceneMeshData.normalAddress;
    m_gpuSceneUboData.uvBufferAddress     = m_gpuSceneMeshData.uvAddress;
    m_gpuSceneUboData.colorBufferAddress  = m_gpuSceneMeshData.colorAddress;
}

void WingEngine::DestroyGPUScene() {
    m_gfx.WaitIdle();
    if (m_bSceneLoaded) {
        if (m_bRayTracingAvailable) {
            LOG_DEBUG(WING, "Destroying RT acceleration structures");
            m_ASManager.DestroyAS();
            LOG_DEBUG(WING, "RT acceleration structures destroyed");
        }

        DestroyGPUSceneMeshData();
        DestroyGPUSceneMaterials();
        DestroyGPUSceneTextures();
    }
    m_bSceneLoaded = false;
}

void WingEngine::InitFrameSceneData() {
    jvk::DescriptorLayoutBuilder builder;
    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (m_bRayTracingAvailable) {
        stages |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    }
    m_gpuSceneDataUboDescriptorLayout = builder.Build(m_gfx.ctx, stages);

    for (auto &frame: m_frameData) {
        frame.gpuSceneDataUboDescriptorSet = m_descriptorAllocator.Allocate(m_gfx.ctx, m_gpuSceneDataUboDescriptorLayout);
        frame.gpuSceneDataUBO              = m_gfx.CreateBuffer(
                sizeof(GPUSceneUBOData),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        frame.gpuSceneDataUBOMapping = static_cast<GPUSceneUBOData *>(frame.gpuSceneDataUBO.Map(m_gfx.allocator));

        jvk::DescriptorWriter writer;
        writer.WriteBuffer(0, frame.gpuSceneDataUBO.buffer, sizeof(GPUSceneUBOData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.UpdateSet(m_gfx.ctx, frame.gpuSceneDataUboDescriptorSet);
    }
}

void WingEngine::DestroyFrameSceneData() const {
    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_gpuSceneDataUboDescriptorLayout, nullptr);

    for (auto &frame: m_frameData) {
        frame.gpuSceneDataUBO.Unmap(m_gfx.allocator);
        m_gfx.DestroyBuffer(frame.gpuSceneDataUBO);
    }
}

void WingEngine::InitGridPipeline() {
    // Shaders
    VkShaderModule vertShader;
    if (!jvk::LoadShaderModule("../shaders/grid.vert.spv", m_gfx.ctx, &vertShader)) {
        LOG_FATAL(WING, "Failed to load grid vertex shader");
    }

    VkShaderModule fragShader;
    if (!jvk::LoadShaderModule("../shaders/grid.frag.spv", m_gfx.ctx, &fragShader)) {
        LOG_FATAL(WING, "Failed to load grid fragment shader");
    }

    // Pipeline
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.offset     = 0;
    pushConstantRange.size       = sizeof(GridPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::PipelineLayout();
    layoutInfo.setLayoutCount             = 0;
    layoutInfo.pSetLayouts                = nullptr;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pushConstantRange;

    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_gridPipeline.layout));

    jvk::PipelineBuilder builder;
    builder.SetShaders(vertShader, fragShader);
    builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.SetMultiSamplingNone();
    builder.EnableBlendingAdditive();
    builder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.DisableStencilTest();
    builder.SetColorAttachmentFormat(m_gfx.drawImage.image.imageFormat);
    builder.SetDepthAttachmentFormat(m_gfx.drawImage.depthStencilImage.imageFormat);
    builder.pipelineLayout = m_gridPipeline.layout;

    m_gridPipeline.pipeline = builder.BuildPipeline(m_gfx.ctx);

    vkDestroyShaderModule(m_gfx.ctx, vertShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, fragShader, nullptr);
}

void WingEngine::DestroyGridPipeline() const {
    m_gridPipeline.Destroy(m_gfx.ctx, true);
}

void WingEngine::BuildBLAS() {
    assert(m_scene != nullptr);

    const VkDeviceAddress vertexAddress = m_gpuSceneMeshData.positionAddress;
    const VkDeviceAddress indexAddress  = m_gpuSceneMeshData.indexAddress;

    // For now, we will build a BLAS for each mesh in the scene.
    std::vector<jtx::BLASInput> inputs;
    inputs.reserve(m_scene->meshes.size());

    for (const auto &mesh: m_scene->meshes) {
        jtx::BLASInput input;

        const uint32_t numPrimitives = mesh.numIndices;

        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexData.deviceAddress = vertexAddress;
        triangles.vertexStride             = sizeof(vec3);
        triangles.indexData.deviceAddress  = indexAddress;
        triangles.indexType                = VK_INDEX_TYPE_UINT32;
        triangles.maxVertex                = mesh.numIndices * 3 - 1;
        triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.triangles = triangles;

        // Double-check the offset here -- common point of failure
        VkAccelerationStructureBuildRangeInfoKHR offset;
        offset.firstVertex     = 0;
        offset.primitiveCount  = mesh.numIndices;
        offset.primitiveOffset = mesh.startIndex * sizeof(vec3u);
        offset.transformOffset = 0;

        input.geometry.push_back(geometry);
        input.buildRange.push_back(offset);

        inputs.push_back(input);
    }
    m_ASManager.BuildBLAS(inputs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void WingEngine::BuildTLAS() {
    // We don't really support instances, so we will just create one instance of each BLAS
    // (We also don't support transforms right now, so identity matrix is hardcoded)
    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    tlas.reserve(m_scene->meshes.size());

    for (size_t i = 0; i < m_scene->meshes.size(); ++i) {
        constexpr VkTransformMatrixKHR identity = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f};

        VkAccelerationStructureInstanceKHR inst{};
        inst.transform                              = identity;
        inst.instanceCustomIndex                    = i;
        inst.accelerationStructureReference         = m_ASManager.GetBLASDeviceAddress(i);
        inst.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        inst.mask                                   = 0xFF;
        inst.instanceShaderBindingTableRecordOffset = 0;
        tlas.push_back(inst);
    }
    m_ASManager.BuildTLAS(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void WingEngine::InitRayTracingDescriptors() {
    LOG_DEBUG(WING, "Initializing ray tracing descriptors");

    jvk::DescriptorLayoutBuilder builder;
    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_rtDescriptorSetLayout = builder.Build(m_gfx.ctx, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    m_rtDescriptorSet       = m_descriptorAllocator.Allocate(m_gfx.ctx, m_rtDescriptorSetLayout);

    LOG_DEBUG(WING, "Ray tracing descriptors initialized");
}

void WingEngine::DestroyRayTracingDescriptors() const {
    LOG_DEBUG(WING, "Destroying ray tracing descriptors");

    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_rtDescriptorSetLayout, nullptr);

    LOG_DEBUG(WING, "Ray tracing descriptors destroyed");
}

void WingEngine::InitRayTracingPipeline() {
    LOG_DEBUG(WING, "Initializing ray tracing pipeline");

    VkShaderModule raygenShader;
    if (!jvk::LoadShaderModule("../shaders/raytrace.rgen.spv", m_gfx.ctx, &raygenShader)) {
        LOG_FATAL(WING, "Failed to load raygen shader");
    }

    VkShaderModule missShader;
    if (!jvk::LoadShaderModule("../shaders/raytrace.rmiss.spv", m_gfx.ctx, &missShader)) {
        LOG_FATAL(WING, "Failed to load miss shader");
    }

    VkShaderModule closestHitShader;
    if (!jvk::LoadShaderModule("../shaders/raytrace.rchit.spv", m_gfx.ctx, &closestHitShader)) {
        LOG_FATAL(WING, "Failed to load closest hit shader");
    }

    enum StageIndices {
        STAGE_RAYGEN,
        STAGE_MISS,
        STAGE_CLOSESTHIT,
        STAGE_SHADERGROUPCOUNT
    };

    std::array<VkPipelineShaderStageCreateInfo, STAGE_SHADERGROUPCOUNT> stages{};
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pName = "main";

    // Raygen
    stage.module         = raygenShader;
    stage.stage          = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages[STAGE_RAYGEN] = stage;

    // Miss
    stage.module       = missShader;
    stage.stage        = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages[STAGE_MISS] = stage;

    // Closest Hit
    stage.module             = closestHitShader;
    stage.stage              = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[STAGE_CLOSESTHIT] = stage;

    // Shader groups
    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.anyHitShader       = VK_SHADER_UNUSED_KHR;// Runs on any potential intersection
    group.closestHitShader   = VK_SHADER_UNUSED_KHR;// Runs on the closest hit point along ray
    group.generalShader      = VK_SHADER_UNUSED_KHR;// Raygen / miss shaders
    group.intersectionShader = VK_SHADER_UNUSED_KHR;// Computes custom ray-geometry intersection

    // Raygen
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = STAGE_RAYGEN;
    m_rtShaderGroups.push_back(group);

    // Miss
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = STAGE_MISS;
    m_rtShaderGroups.push_back(group);

    // Closest Hit
    group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader    = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = STAGE_CLOSESTHIT;
    m_rtShaderGroups.push_back(group);

    // Pipeline layout
    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // -- Push constants
    VkPushConstantRange pcRange{};
    pcRange.stageFlags            = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
    pcRange.offset                = 0;
    pcRange.size                  = sizeof(GPURayTracingPushConstants);
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges    = &pcRange;

    // -- Descriptor sets
    const std::vector descriptorLayouts{m_rtDescriptorSetLayout, m_gpuSceneDataUboDescriptorLayout};
    layout.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
    layout.pSetLayouts    = descriptorLayouts.data();

    vkCreatePipelineLayout(m_gfx.ctx, &layout, nullptr, &m_rtPipelineLayout);

    // RT Pipeline
    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount                   = static_cast<uint32_t>(stages.size());// i.e. # of shaders
    pipelineInfo.pStages                      = stages.data();
    pipelineInfo.groupCount                   = static_cast<uint32_t>(m_rtShaderGroups.size());
    pipelineInfo.pGroups                      = m_rtShaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout                       = m_rtPipelineLayout;
    vkCreateRayTracingPipelinesKHR(m_gfx.ctx, {}, {}, 1, &pipelineInfo, nullptr, &m_rtPipeline);

    vkDestroyShaderModule(m_gfx.ctx, raygenShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, missShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, closestHitShader, nullptr);

    LOG_DEBUG(WING, "Ray tracing pipeline initialized");
}

void WingEngine::DestroyRayTracingPipeline() const {
    LOG_DEBUG(WING, "Destroying ray tracing pipeline");

    vkDestroyPipeline(m_gfx.ctx, m_rtPipeline, nullptr);
    vkDestroyPipelineLayout(m_gfx.ctx, m_rtPipelineLayout, nullptr);

    LOG_DEBUG(WING, "Ray tracing pipeline destroyed");
}

inline uint32_t AlignUp(const uint32_t size, const uint32_t alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}

void WingEngine::InitRayTracingSBT() {
    LOG_DEBUG(WING, "Initializing SBT");

    constexpr uint32_t rayGenCount = 1;
    constexpr uint32_t missCount   = 1;
    constexpr uint32_t hitCount    = 1;
    constexpr auto handleCount     = rayGenCount + missCount + hitCount;
    const uint32_t handleSize      = m_gfx.rtProperties.shaderGroupHandleSize;

    // TLDR: describing how to traverse the table for each type of shader
    // Stride is the size of the handle aligned to shaderGroupHandleAlignment (except ray gen)
    // Size of group is # of handles (aligned at shaderGroupHandleAlignment)
    const uint32_t handleSizeAligned = AlignUp(handleSize, m_gfx.rtProperties.shaderGroupHandleAlignment);

    m_rayGenRegion.stride = AlignUp(rayGenCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);
    m_rayGenRegion.size   = m_rayGenRegion.stride;

    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size   = AlignUp(missCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);

    m_hitRegion.stride = handleSizeAligned;
    m_hitRegion.size   = AlignUp(hitCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);

    // Shader group handles (where to access them in the pipeline)
    const uint32_t dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    CHECK_VK(vkGetRayTracingShaderGroupHandlesKHR(m_gfx.ctx, m_rtPipeline, 0, handleCount, dataSize, handles.data()));

    // Allocate buffer
    const VkDeviceSize sbtSize                    = m_rayGenRegion.size + m_missRegion.size + m_hitRegion.size + m_callableRegion.size;
    constexpr VkBufferUsageFlags bufferFlags      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    constexpr VmaMemoryUsage memUsage             = VMA_MEMORY_USAGE_CPU_TO_GPU;
    constexpr VmaAllocationCreateFlags allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_rtSBTBuffer                                 = m_gfx.CreateBuffer(sbtSize, bufferFlags, memUsage, allocFlags);

    const VkDeviceAddress sbtAddress = m_rtSBTBuffer.GetDeviceAddress(m_gfx.ctx);
    m_rayGenRegion.deviceAddress     = sbtAddress;
    m_missRegion.deviceAddress       = sbtAddress + m_rayGenRegion.size;
    m_hitRegion.deviceAddress        = sbtAddress + m_rayGenRegion.size + m_missRegion.size;

    const auto GetHandle = [&](const int i) { return handles.data() + i * handleSize; };

    // Copy the handles retrieved from the pipeline into the buffer
    auto *pSbtBuffer   = static_cast<uint8_t *>(m_rtSBTBuffer.Map(m_gfx.allocator));
    uint8_t *pData     = nullptr;
    uint32_t handleIdx = 0;

    // Raygen
    pData = pSbtBuffer;
    memcpy(pData, GetHandle(handleIdx++), handleSize);

    pData = pSbtBuffer + m_rayGenRegion.size;
    for (uint32_t c = 0; c < missCount; ++c) {
        memcpy(pData, GetHandle(handleIdx++), handleSize);
        pData += m_missRegion.stride;
    }

    pData = pSbtBuffer + m_rayGenRegion.size + m_missRegion.size;
    for (uint32_t c = 0; c < hitCount; ++c) {
        memcpy(pData, GetHandle(handleIdx++), handleSize);
        pData += m_hitRegion.stride;
    }

    m_rtSBTBuffer.Unmap(m_gfx.allocator);

    LOG_DEBUG(WING, "SBT initialized");
}

void WingEngine::DestroyRayTracingSBT() const {
    LOG_DEBUG(WING, "Destroying SBT");

    m_gfx.DestroyBuffer(m_rtSBTBuffer);

    LOG_DEBUG(WING, "SBT destroyed");
}

void WingEngine::RayTrace(RenderContext &ctx, const glm::vec4 &clearColor) const {
    jvk::TransitionImageIfNeeded(ctx.cmd, ctx.drawImage.image, ctx.layout.drawImage, VK_IMAGE_LAYOUT_GENERAL);
    ctx.layout.drawImage = VK_IMAGE_LAYOUT_GENERAL;

    GPURayTracingPushConstants pc{};
    pc.clearColor     = clearColor;
    pc.lightPosition  = glm::vec3(10.0f, 10.0f, 10.0f);
    pc.lightIntensity = 10.0f;
    pc.lightType      = 0;

    const auto sceneDescriptorSet = m_frameData[m_gfx.GetCurrentFrameIndex()].gpuSceneDataUboDescriptorSet;
    const std::vector descriptorSets{m_rtDescriptorSet, sceneDescriptorSet};
    vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
    vkCmdBindDescriptorSets(
        ctx.cmd,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        m_rtPipelineLayout,
        0,
        (uint32_t)descriptorSets.size(),
        descriptorSets.data(),
        0,
        nullptr);
    vkCmdPushConstants(
        ctx.cmd,
        m_rtPipelineLayout,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
        0,
        sizeof(GPURayTracingPushConstants),
        &pc);
    vkCmdTraceRaysKHR(ctx.cmd, &m_rayGenRegion, &m_missRegion, &m_hitRegion, &m_callableRegion, m_viewRectangle.w, m_viewRectangle.h, 1);
}

}// namespace jtx