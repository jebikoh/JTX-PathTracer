#include <engine/vulkan/vk_engine.hpp>
#include <interface/display.hpp>
#include <interface/ui_renderer.hpp>
#include <jvk/shaders.hpp>
#include <scene/scene.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <engine/vulkan/rt.hpp>

#include <glm/gtx/transform.hpp>

namespace jtx {

void VkEngine::Init(const bool bEnableRayTracing) {
    LOG_INFO(VKE, "Initializing Vulkan engine");
    m_bRayTracingAvailable = bEnableRayTracing;

    InitDescriptors();
    InitPipelines();
    if (m_bRayTracingAvailable) {
        InitRayTracingSBT();
    }

    LOG_INFO(VKE, "Vulkan engine initialized");
}

void VkEngine::Destroy() {
    LOG_INFO(VKE, "Destroying Vulkan engine");

    if (m_bRayTracingAvailable) {
        DestroyRayTracingSBT();
    }

    DestroyScene();
    DestroyPipelines();
    DestroyDescriptors();

    LOG_INFO(VKE, "Vulkan engine destroyed");
}

void VkEngine::Draw(RenderContext &ctx, ResolveRegion &region) {
    // Reset frame count if ray tracing was just enabled
    if (m_bRayTracingEnabled && !m_bRayTracingEnabledPreviousFrame) m_rtFrameNumber = -1;
    m_bRayTracingEnabledPreviousFrame = m_bRayTracingEnabled;

    if (m_camera.HasChanged()) {
        m_camera.Update();
        m_rtFrameNumber = -1;
    }
    m_rtFrameNumber++;

    UpdateGlobalUniformData();

    if (m_bSceneLoaded) {
        PopulateContext();
    }

    // Calculate viewport
    const VkRect2D renderArea{
            {m_viewRectangle.x, m_viewRectangle.y},
            {m_viewRectangle.w, m_viewRectangle.h}};

    region.dst[0].width = m_viewRectangle.x;
    region.dst[0].height = m_viewRectangle.y;
    region.dst[1].width = m_viewRectangle.x + m_viewRectangle.w;
    region.dst[1].height = m_viewRectangle.y + m_viewRectangle.h;

    // TODO: rework ResolveRegion
    if (m_bRayTracingEnabled) {
        region.src[0] = {0, 0};
        region.src[1].width = m_viewRectangle.w;
        region.src[1].height = m_viewRectangle.h;
        region.target = kRenderTarget::DRAW32f;
        RayTrace(ctx, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    } else {
        region.src[0] = region.dst[0];
        region.src[1] = region.dst[1];
        region.target = kRenderTarget::DRAW16f;
        Rasterize(ctx, renderArea);
    }
}

void VkEngine::PopulateContext() {
    m_drawContext.objects.clear();

    // Loop through meshes
    for (uint32_t i = 0; i < m_pScene->meshes.size(); ++i) {
        const auto &mesh = m_pScene->meshes[i];
        GPURenderObject obj{};
        obj.objectID = i;
        obj.start    = mesh.startIndex;
        obj.count    = mesh.numIndices;
        obj.materialPipeline = &m_materialPipelines.diffuse;
        m_drawContext.objects.push_back(obj);
    }
}

void VkEngine::Rasterize(RenderContext &ctx, const VkRect2D &renderArea) {
    // Begin render pass
    VkClearValue drawImageClearValue{};
    drawImageClearValue.color = {0.255f, 0.247f, 0.255f, 1.0f};

    const auto &drawImage = m_gfx.targets.draw16f;
    const auto &depthStencil = m_gfx.targets.depthStencil;

    // Transition if needed
    jvk::TransitionImageIfNeeded(ctx.cmd, drawImage, ctx.layout.draw16f, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.layout.draw16f = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    jvk::TransitionImageIfNeeded(ctx.cmd, depthStencil.image, ctx.layout.depthStencil, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    ctx.layout.depthStencil = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const VkRenderingAttachmentInfo colorAttachment = jvk::init::RenderingAttachment(drawImage.view, &drawImageClearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue dsClearValue{};
    dsClearValue.depthStencil.depth   = 1.0f;
    dsClearValue.depthStencil.stencil = 0;

    const VkRenderingAttachmentInfo depthAttachment = jvk::init::DepthRenderingAttachment(depthStencil.view, &dsClearValue, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo                   = jvk::init::Rendering(ctx.swapchain.extent, &colorAttachment, &depthAttachment);
    renderingInfo.renderArea                        = renderArea;

    vkCmdBeginRenderingKHR(ctx.cmd, &renderingInfo);

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

    const FrameData &frame = m_frameData[ctx.frameIndex];

    if (m_bSceneLoaded) {
        // Sort the draws by pipline
        std::vector<uint32_t> opaqueDraws;
        opaqueDraws.reserve(m_drawContext.objects.size());
        for (uint32_t i = 0; i < m_drawContext.objects.size(); i++) {
            opaqueDraws.push_back(i);
        }

        std::ranges::sort(opaqueDraws, [&](const auto &iA, const auto &iB) {
            const GPURenderObject &a = m_drawContext.objects[iA];
            const GPURenderObject &b = m_drawContext.objects[iB];
            if (a.materialPipeline == b.materialPipeline) {
                return a.start < b.start;
            }
            return a.materialPipeline < b.materialPipeline;
        });

        // Update scene data UBO
        *frame.gpuGlobalUniformDataMapping = m_gpuGlobalUniformData;

        // Bind layouts
        vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materialPipelines.layout, 0, 1, &frame.gpuGlobalUniformDataDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materialPipelines.layout, 1, 1, &m_bindlessDescriptorSet, 0, nullptr);

        // Bind index buffer
        vkCmdBindIndexBuffer(ctx.cmd, m_gpuSceneData.index, 0, VK_INDEX_TYPE_UINT32);

        const VkPipeline *lastPipeline = nullptr;
        auto draw                      = [&](const GPURenderObject &r) {
            if (r.materialPipeline != lastPipeline) {
                lastPipeline = r.materialPipeline;

                vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *r.materialPipeline);
            }

            DrawPushConstants pushConstants{};
            pushConstants.objectID = r.objectID;
            vkCmdPushConstants(ctx.cmd, m_materialPipelines.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

            // Need to multiply by 3 because r.count and r.start are relative to vec3u
            vkCmdDrawIndexed(ctx.cmd, r.count * 3, 1, r.start * 3, 0, 0);
        };

        for (const auto &r: opaqueDraws) {
            draw(m_drawContext.objects[r]);
        }
    }

    if (m_bDrawGrid) {
        vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gridPipeline.pipeline);

        GridPushConstants pushConstants{};
        pushConstants.viewProj    = m_gpuGlobalUniformData.viewProj;
        pushConstants.cameraPos   = m_gpuGlobalUniformData.cameraPosition;
        pushConstants.invViewProj = m_gpuGlobalUniformData.invViewProj;
        vkCmdPushConstants(ctx.cmd, m_gridPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(ctx.cmd, 3, 1, 0, 0);
    }

    vkCmdEndRenderingKHR(ctx.cmd);
}

void VkEngine::ProcessEvent(const SDL_Event &event) {
    m_camera.ProcessSDLEvent(event);
}

void VkEngine::SkipEvent() {
    m_camera.ResetInputState();
}

void VkEngine::DrawSettingsPanel(UiDrawContext &ctx) {
    ctx.StartRectangleBackground();
    if (ctx.StartTable("VkRasterizationTable")) {
        ctx.NewRow("Rasterization");
        ctx.NewRow("Draw grid");
        ImGui::Checkbox("##Grid", &m_bDrawGrid);
        ctx.EndTable();
    }

    ImGui::Separator();

    if (ctx.StartTable("VkRayTracingTable")) {
        if (!m_bRayTracingAvailable) ImGui::BeginDisabled();
        ctx.NewRow("Ray Tracing");
        ctx.NewRow("Enable");
        ImGui::Checkbox("##RT", &m_bRayTracingEnabled);

        ctx.NewRow("Max frames");
        int32_t maxFrames = m_rtMaxFrames;
        if (ImGui::DragInt("##MaxFrames", &maxFrames)) {
            m_rtMaxFrames = static_cast<uint32_t>(maxFrames);
        }

        ctx.NewRow("Samples Per Frame");
        int32_t samplePerFrame = m_rtSamplesPerFrame;
        if (ImGui::DragInt("##SamplesPerFrame", &samplePerFrame, 1, 1, 32)) {
            m_rtSamplesPerFrame = static_cast<uint32_t>(samplePerFrame);
        }

        if (!m_bRayTracingAvailable) ImGui::EndDisabled();
        ctx.EndTable();
    }

    ctx.EndRectangleBackground();
}

void VkEngine::InitDescriptors() {
    // -- Bindless descriptor pool --
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 512},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512}};
    if (m_bRayTracingAvailable) {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1});
    }

    m_bindlessAllocator.InitPool(m_gfx.ctx, 4, poolSizes, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

    // -- Bindless descriptor set layout --
    VkShaderStageFlags shaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (m_bRayTracingAvailable) {
        shaderStages |= VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    }

    jvk::DescriptorLayoutBuilder builder;
    builder.AddBinding(kL2Bindings::GPU_OBJECT_DATA, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStages);
    builder.AddBinding(kL2Bindings::GPU_MATERIAL_DATA, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStages);
    builder.AddBinding(kL2Bindings::GPU_TEXTURE_SAMPLER_ARRAY, 256, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStages);
    if (m_bRayTracingAvailable) {
        // Should NOT be available during rasterization
        builder.AddBinding(kL2Bindings::GPU_TLAS, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    }

    constexpr VkDescriptorBindingFlags bindingFlags     = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    std::vector<VkDescriptorBindingFlags> vBindingFlags = {
            bindingFlags,
            bindingFlags,
            bindingFlags,
    };
    if (m_bRayTracingAvailable) {
        vBindingFlags.push_back(bindingFlags);
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    bindingFlagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount  = static_cast<uint32_t>(vBindingFlags.size());
    bindingFlagsInfo.pBindingFlags = vBindingFlags.data();

    m_bindlessDescriptorSetLayout = builder.Build(m_gfx.ctx, &bindingFlagsInfo, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

    // -- Bindless descriptor set --
    m_bindlessDescriptorSet = m_bindlessAllocator.Allocate(m_gfx.ctx, m_bindlessDescriptorSetLayout);

    // -- Global uniform data descriptor set allocator --
    std::vector<VkDescriptorPoolSize> poolSizesGlobal = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
    m_descriptorAllocator.InitPool(m_gfx.ctx, 2, poolSizesGlobal);

    builder.Clear();

    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStages);
    // Raygen needs access to the draw image
    if (m_bRayTracingAvailable) {
        builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    }

    m_gpuGlobalUniformDataDescriptorLayout = builder.Build(m_gfx.ctx);

    for (auto &frame: m_frameData) {
        frame.gpuGlobalUniformDataDescriptorSet = m_descriptorAllocator.Allocate(m_gfx.ctx, m_gpuGlobalUniformDataDescriptorLayout);
    }
}

void VkEngine::DestroyDescriptors() const {
    LOG_DEBUG(VKE, "Destroying descriptors");

    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_bindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_gpuGlobalUniformDataDescriptorLayout, nullptr);

    m_bindlessAllocator.DestroyPool(m_gfx.ctx);
    m_descriptorAllocator.DestroyPool(m_gfx.ctx);

    LOG_DEBUG(VKE, "Descriptors destroyed");
}

void VkEngine::UpdateGlobalUniformData() {
    float aspectRatio;
    if (m_viewRectangle.w > 0 && m_viewRectangle.h > 0) {
        aspectRatio = static_cast<float>(m_viewRectangle.w) / static_cast<float>(m_viewRectangle.h);
    } else {
        aspectRatio = static_cast<float>(m_gfx.window.extent.width) / static_cast<float>(m_gfx.window.extent.width);
    }

    m_cache.view = m_camera.GetViewMatrix();
    m_cache.proj = glm::perspective(glm::radians(70.f), aspectRatio, 0.1f, 10000.0f);
    m_cache.proj[1][1] *= -1;

    m_gpuGlobalUniformData.viewProj       = m_cache.proj * m_cache.view;
    m_gpuGlobalUniformData.invViewProj    = glm::inverse(m_gpuGlobalUniformData.viewProj);
    m_gpuGlobalUniformData.cameraPosition = glm::vec4(m_camera.position, 0.0f);
    m_gpuGlobalUniformData.sunDirection   = glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f);
    m_gpuGlobalUniformData.sunIntensity   = 10.0f;
    m_gpuGlobalUniformData.vertexBuffer   = m_gpuSceneData.positionAddress;
    m_gpuGlobalUniformData.normalBuffer   = m_gpuSceneData.normalAddress;
    m_gpuGlobalUniformData.texCoordBuffer       = m_gpuSceneData.uvAddress;
    m_gpuGlobalUniformData.colorBuffer    = m_gpuSceneData.colorAddress;
    m_gpuGlobalUniformData.indexBuffer    = m_gpuSceneData.indexAddress;
}

void VkEngine::InitPipelines() {
    LOG_DEBUG(VKE, "Initializing Pipelines");

    InitMaterialPipelines();
    InitGridPipeline();
    if (m_bRayTracingAvailable) {
        InitRayTracingPipeline();
    }

    LOG_DEBUG(VKE, "Pipelines initialized");
}

void VkEngine::DestroyPipelines() const {
    LOG_DEBUG(VKE, "Destroying Pipelines");

    if (m_bRayTracingAvailable) {
        DestroyRayTracingPipeline();
    }
    DestroyGridPipeline();
    DestroyMaterialPipelines();

    LOG_DEBUG(VKE, "Pipelines destroyed");
}

void VkEngine::InitMaterialPipelines() {
    VkShaderModule vertexShader;
    if (!jvk::LoadShaderModule("spv/mesh.vert.spv", m_gfx.ctx, &vertexShader)) {
        LOG_FATAL(VKE, "Failed to load vertex shader");
    }

    VkShaderModule diffuseFragmentShader;
    if (!jvk::LoadShaderModule("spv/diffuse.frag.spv", m_gfx.ctx, &diffuseFragmentShader)) {
        LOG_FATAL(VKE, "Failed to load diffuse fragment shader");
    }

    VkPushConstantRange pc{};
    pc.offset     = 0;
    pc.size       = sizeof(DrawPushConstants);
    pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    const VkDescriptorSetLayout descriptorSetLayouts[] = {m_gpuGlobalUniformDataDescriptorLayout, m_bindlessDescriptorSetLayout};

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::PipelineLayout();
    layoutInfo.setLayoutCount             = 2;
    layoutInfo.pSetLayouts                = descriptorSetLayouts;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pc;
    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_materialPipelines.layout));

    jvk::PipelineBuilder builder;
    builder.SetShaders(vertexShader, diffuseFragmentShader);
    builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.SetMultiSamplingNone();
    builder.DisableBlending();
    builder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.DisableStencilTest();
    builder.SetColorAttachmentFormat(m_gfx.targets.draw16f.format);
    builder.SetDepthAttachmentFormat(m_gfx.targets.depthStencil.format);
    builder.pipelineLayout = m_materialPipelines.layout;

    m_materialPipelines.diffuse = builder.BuildPipeline(m_gfx.ctx);

    // Create more pipelines as needed here

    vkDestroyShaderModule(m_gfx.ctx, vertexShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, diffuseFragmentShader, nullptr);
}

void VkEngine::DestroyMaterialPipelines() const {
    vkDestroyPipelineLayout(m_gfx.ctx, m_materialPipelines.layout, nullptr);
    vkDestroyPipeline(m_gfx.ctx, m_materialPipelines.diffuse, nullptr);
}

void VkEngine::InitGridPipeline() {
    VkShaderModule vertexShader;
    if (!jvk::LoadShaderModule("spv/grid.vert.spv", m_gfx.ctx, &vertexShader)) {
        LOG_FATAL(VKE, "Failed to load grid vertex shader");
    }

    VkShaderModule fragmentShader;
    if (!jvk::LoadShaderModule("spv/grid.frag.spv", m_gfx.ctx, &fragmentShader)) {
        LOG_FATAL(VKE, "Failed to load grid fragment shader");
    }

    VkPushConstantRange pc{};
    pc.offset     = 0;
    pc.size       = sizeof(GridPushConstants);
    pc.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::PipelineLayout();
    layoutInfo.setLayoutCount             = 0;
    layoutInfo.pSetLayouts                = nullptr;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pc;

    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_gridPipeline.layout));

    jvk::PipelineBuilder builder;
    builder.SetShaders(vertexShader, fragmentShader);
    builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.SetMultiSamplingNone();
    builder.EnableBlendingAdditive();
    builder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.DisableStencilTest();
    builder.SetColorAttachmentFormat(m_gfx.targets.draw16f.format);
    builder.SetDepthAttachmentFormat(m_gfx.targets.depthStencil.format);
    builder.pipelineLayout  = m_gridPipeline.layout;
    m_gridPipeline.pipeline = builder.BuildPipeline(m_gfx.ctx);

    vkDestroyShaderModule(m_gfx.ctx, vertexShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, fragmentShader, nullptr);
}

void VkEngine::DestroyGridPipeline() const {
    m_gridPipeline.Destroy(m_gfx.ctx, true);
}

void VkEngine::LoadScene(const Scene *pScene) {
    LOG_INFO(VKE, "Loading scene");
    if (m_bSceneLoaded) {
        DestroyScene();
    }
    m_pScene = pScene;

    jvk::DescriptorWriter writer;

    // -- Global data --
    LOG_DEBUG(VKE, "Preparing global uniform data buffers");
    for (auto &frame: m_frameData) {
        if (frame.gpuGlobalUniformData.buffer == VK_NULL_HANDLE) {
            frame.gpuGlobalUniformData = m_gfx.CreateBuffer(
                    sizeof(GPUGlobalUniformData),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            frame.gpuGlobalUniformDataMapping = static_cast<GPUGlobalUniformData *>(frame.gpuGlobalUniformData.Map(m_gfx.allocator));

            writer.WriteBuffer(0, frame.gpuGlobalUniformData.buffer, sizeof(GPUGlobalUniformData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            if (m_bRayTracingAvailable) {
                writer.WriteImage(1, m_gfx.targets.draw32f.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            }
            writer.UpdateSet(m_gfx.ctx, frame.gpuGlobalUniformDataDescriptorSet);
            writer.Clear();
        }
        *frame.gpuGlobalUniformDataMapping = {};
    }
    LOG_DEBUG(VKE, "Global uniform data buffers prepared");

    // -- Textures --
    LOG_DEBUG(VKE, "Loading textures");
    m_gpuSceneData.textures.resize(pScene->textures.size());
    LOG_DEBUG(VKE, "    Scene has {} textures", pScene->textures.size());
    uint32_t index = 0;
    for (const auto &tex: pScene->textures) {
        if (tex.IsEmpty()) {
            ++index;
            continue;
        }

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

        writer.WriteImage(
                kL2Bindings::GPU_TEXTURE_SAMPLER_ARRAY,
                index, gpuTex.view,
                m_gfx.defaultSamplers.linear,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        m_gpuSceneData.textures[index++] = gpuTex;
    }

    LOG_DEBUG(VKE, "Textures loaded");

    // -- Mesh buffers --
    // Calculate non-interleaved buffer sizes
    LOG_DEBUG(VKE, "Loading mesh data");
    const size_t positionBufferSize = pScene->positions.size() * sizeof(vec3);
    const size_t normalBufferSize   = pScene->normals.size() * sizeof(vec3);
    const size_t uvBufferSize       = pScene->texCoords.size() * sizeof(vec2);
    const size_t colorBufferSize    = pScene->colors.size() * sizeof(vec3);
    const size_t indexBufferSize    = pScene->indices.size() * sizeof(vec3u);
    const size_t totalSize          = positionBufferSize + normalBufferSize + uvBufferSize + colorBufferSize + indexBufferSize;

    bool bSceneHasVertexColors = colorBufferSize > 0;

    // Vertex buffers (position, normal, uv, color)
    VkBufferUsageFlags vertexBufferUsages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (m_bRayTracingAvailable) {
        vertexBufferUsages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    constexpr VmaMemoryUsage bufferMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_gpuSceneData.position = m_gfx.CreateBuffer(positionBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(VKE, "    Created position GPU buffer of size {} bytes", positionBufferSize);
    m_gpuSceneData.normal = m_gfx.CreateBuffer(normalBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(VKE, "    Created normal GPU buffer of size {} bytes", normalBufferSize);
    m_gpuSceneData.uv = m_gfx.CreateBuffer(uvBufferSize, vertexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(VKE, "    Created UV GPU buffer of size {} bytes", uvBufferSize);
    if (bSceneHasVertexColors) {
        m_gpuSceneData.color = m_gfx.CreateBuffer(colorBufferSize, vertexBufferUsages, bufferMemoryUsage);
        LOG_DEBUG(VKE, "    Created color GPU buffer of size {} bytes", colorBufferSize);
    }

    // Index buffer
    // We need VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT for ray tracing
    VkBufferUsageFlags indexBufferUsages = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (m_bRayTracingAvailable) {
        indexBufferUsages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    m_gpuSceneData.index = m_gfx.CreateBuffer(indexBufferSize, indexBufferUsages, bufferMemoryUsage);
    LOG_DEBUG(VKE, "    Created index buffer of size {} bytes", indexBufferSize);

    // Device addresses
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    m_gpuSceneData.indexAddress    = m_gpuSceneData.index.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneData.positionAddress = m_gpuSceneData.position.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneData.normalAddress   = m_gpuSceneData.normal.GetDeviceAddress(m_gfx.ctx);
    m_gpuSceneData.uvAddress       = m_gpuSceneData.uv.GetDeviceAddress(m_gfx.ctx);
    if (bSceneHasVertexColors) {
        m_gpuSceneData.colorAddress = m_gpuSceneData.color.GetDeviceAddress(m_gfx.ctx);
    }

    // Staging buffer
    jvk::Buffer staging = m_gfx.CreateBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(VKE, "    Created staging buffer of size {} bytes", totalSize);

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
    LOG_DEBUG(VKE, "    Vertex data copied to staging buffer");

    // Copy to GPU
    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.dstOffset = 0;

        copyRegion.srcOffset = 0;
        copyRegion.size      = indexBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.index.buffer, 1, &copyRegion);

        copyRegion.srcOffset += indexBufferSize;
        copyRegion.size = positionBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.position.buffer, 1, &copyRegion);

        copyRegion.srcOffset += positionBufferSize;
        copyRegion.size = normalBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.normal.buffer, 1, &copyRegion);

        copyRegion.srcOffset += normalBufferSize;
        copyRegion.size = uvBufferSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.uv.buffer, 1, &copyRegion);

        if (bSceneHasVertexColors) {
            copyRegion.srcOffset += uvBufferSize;
            copyRegion.size = colorBufferSize;
            vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.color.buffer, 1, &copyRegion);
        }
    });
    m_gfx.DestroyBuffer(staging);
    LOG_DEBUG(VKE, "    Staging buffer copied to GPU buffers");
    LOG_DEBUG(VKE, "Mesh data loaded");

    // -- Materials --
    LOG_DEBUG(VKE, "Loading material data");
    std::vector<GPUMaterialData> gpuMaterials;
    gpuMaterials.reserve(pScene->materials.size());

    LOG_DEBUG(VKE, "    Scene has {} materials", pScene->materials.size());
    m_gpuSceneData.materialBuffer = m_gfx.CreateBuffer(sizeof(GPUMaterialData) * pScene->materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    LOG_DEBUG(VKE, "    Created material buffer of size {} bytes", sizeof(GPUMaterialData) * pScene->materials.size());

    void *materialData = m_gpuSceneData.materialBuffer.Map(m_gfx.allocator);

    offset = 0;
    for (const auto &material: pScene->materials) {
        GPUMaterialData m{};
        m.diffuse                                              = vec4(material.parameters.diffuse, 0.0f);
        m.ior                                                  = vec4(material.parameters.ior, 0.0f);
        m.k                                                    = vec4(material.parameters.k, 0.0f);
        m.f0                                                   = vec4(material.parameters.f0, 0.0f);
        m.emission                                             = vec4(material.parameters.emission, 0.0f);
        m.roughness                                            = vec4(vec3(material.parameters.roughness, 0.0f), 0.0f);
        m.diffuseTexture                                       = material.textureIndices.diffuse;
        static_cast<GPUMaterialData *>(materialData)[offset++] = m;
    }

    m_gpuSceneData.materialBuffer.Unmap(m_gfx.allocator);
    LOG_DEBUG(VKE, "    Material data copied to buffer");
    writer.WriteBuffer(kL2Bindings::GPU_MATERIAL_DATA, m_gpuSceneData.materialBuffer.buffer, sizeof(GPUMaterialData) * pScene->materials.size(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    LOG_DEBUG(VKE, "Material data loaded");

    // -- Objects --
    LOG_DEBUG(VKE, "Loading objects");
    std::vector<GPUObjectData> gpuObjects;
    for (const auto &mesh: pScene->meshes) {
        GPUObjectData obj;
        // We don't support transformation matrices for now
        obj.world  = glm::mat4(1.0f);
        obj.normal = glm::mat4(1.0f);
        // Resource handles should align with materialIndex
        obj.startIndex = mesh.startIndex;
        obj.material = mesh.materialIndex;
        gpuObjects.push_back(obj);

        GPURenderObject rObj{};
        rObj.start = mesh.startIndex;
        rObj.count = mesh.numIndices;
        // TODO: assign this dynamically
        rObj.materialPipeline = &m_materialPipelines.diffuse;
    }
    size_t objSize              = sizeof(GPUObjectData) * gpuObjects.size();
    m_gpuSceneData.objectBuffer = m_gfx.CreateBuffer(objSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    LOG_DEBUG(VKE, "   Created GPU objects buffer of size {} bytes", objSize);

    staging = m_gfx.CreateBuffer(objSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(VKE, "   Created staging buffer  of size {} bytes", objSize);
    memcpy(staging.GetMapping(), gpuObjects.data(), objSize);
    LOG_DEBUG(VKE, "   Copied objects to staging buffer");


    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.dstOffset = 0;
        copyRegion.srcOffset = 0;
        copyRegion.size      = objSize;
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.objectBuffer.buffer, 1, &copyRegion);
    });
    m_gfx.DestroyBuffer(staging);
    LOG_DEBUG(VKE, "   Copied data from staging buffer to GPU objects buffer");

    writer.WriteBuffer(kL2Bindings::GPU_OBJECT_DATA, m_gpuSceneData.objectBuffer.buffer, objSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    LOG_DEBUG(VKE, "Objects loaded");

    // -- TLAS --
    if (m_bRayTracingAvailable) {
        LOG_DEBUG(VKE, "Initializing RT scene resources");
        BuildBLAS();
        BuildTLAS();

        writer.WriteAS(kL2Bindings::GPU_TLAS, m_ASManager.GetTLAS());

        LOG_DEBUG(VKE, "RT scene resources initialized");
    }

    // -- Update set --
    writer.UpdateSet(m_gfx.ctx, m_bindlessDescriptorSet);
    m_bSceneLoaded = true;

    LOG_INFO(VKE, "Scene loaded");
}

void VkEngine::DestroyScene() {
    if (m_bSceneLoaded) {
        LOG_INFO(VKE, "Destroying scene");
        m_gfx.WaitIdle();

        // -- RT resources --
        if (m_bRayTracingAvailable) {
            LOG_DEBUG(VKE, "Destroying RT acceleration structures");
            m_ASManager.DestroyAS();
            LOG_DEBUG(VKE, "RT acceleration structures destroyed");
        }

        // -- Objects --
        m_gfx.DestroyBuffer(m_gpuSceneData.objectBuffer);
        LOG_DEBUG(VKE, "GPU objects destroyed");

        // -- Material resources --
        m_gfx.DestroyBuffer(m_gpuSceneData.materialBuffer);
        LOG_DEBUG(VKE, "Material data destroyed");

        // -- Mesh buffers --
        LOG_DEBUG(VKE, "Destroying mesh data");
        LOG_DEBUG(VKE, "    Destroying index buffer");
        m_gfx.DestroyBuffer(m_gpuSceneData.index);
        LOG_DEBUG(VKE, "    Destroying vertex buffers");
        m_gfx.DestroyBuffer(m_gpuSceneData.position);
        LOG_DEBUG(VKE, "    Destroying normal buffer");
        m_gfx.DestroyBuffer(m_gpuSceneData.normal);
        LOG_DEBUG(VKE, "    Destroying UV buffer");
        m_gfx.DestroyBuffer(m_gpuSceneData.uv);
        if (m_gpuSceneData.color.IsValid()) {
            LOG_DEBUG(VKE, "    Destroying color buffer");
            m_gfx.DestroyBuffer(m_gpuSceneData.color);
        }
        LOG_DEBUG(VKE, "Destroyed mesh data");

        // -- Textures --
        for (const auto &tex: m_gpuSceneData.textures) {
            m_gfx.DestroyImage(tex);
        }
        m_gpuSceneData.textures.clear();
        LOG_DEBUG(VKE, "Destroyed textures");

        // -- Global data --
        for (auto &frame: m_frameData) {
            frame.gpuGlobalUniformData.Unmap(m_gfx.allocator);
            m_gfx.DestroyBuffer(frame.gpuGlobalUniformData);
        }
        LOG_DEBUG(VKE, "Destroyed global uniform data buffers");

        m_pScene = nullptr;
        LOG_INFO(VKE, "Scene destroyed");
    }
    m_bSceneLoaded = false;
}

void VkEngine::BuildBLAS() {
    assert(m_pScene != nullptr);

    const VkDeviceAddress vertexAddress = m_gpuSceneData.positionAddress;
    const VkDeviceAddress indexAddress  = m_gpuSceneData.indexAddress;

    // For now, we will build a BLAS for each mesh in the scene.
    std::vector<jtx::BLASInput> inputs;
    inputs.reserve(m_pScene->meshes.size());

    for (const auto &mesh: m_pScene->meshes) {
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

void VkEngine::BuildTLAS() {
    // We don't really support instances, so we will just create one instance of each BLAS
    // (We also don't support transforms right now, so identity matrix is hardcoded)
    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    tlas.reserve(m_pScene->meshes.size());

    for (size_t i = 0; i < m_pScene->meshes.size(); ++i) {
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

void VkEngine::InitRayTracingPipeline() {
    LOG_DEBUG(VKE, "Initializing ray tracing pipeline");

    VkShaderModule raygenShader;
    if (!jvk::LoadShaderModule("spv/raytrace.rgen.spv", m_gfx.ctx, &raygenShader)) {
        LOG_FATAL(VKE, "Failed to load raygen shader");
    }

    VkShaderModule missShader;
    if (!jvk::LoadShaderModule("spv/raytrace.rmiss.spv", m_gfx.ctx, &missShader)) {
        LOG_FATAL(VKE, "Failed to load miss shader");
    }

    VkShaderModule closestHitShader;
    if (!jvk::LoadShaderModule("spv/raytrace.rchit.spv", m_gfx.ctx, &closestHitShader)) {
        LOG_FATAL(VKE, "Failed to load closest hit shader");
    }

    enum StageIndices {
        STAGE_RAYGEN,
        STAGE_MISS,
        STAGE_CLOSEST_HIT,
        STAGE_SHADER_GROUP_COUNT
    };

    std::array<VkPipelineShaderStageCreateInfo, STAGE_SHADER_GROUP_COUNT> stages{};
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
    stage.module              = closestHitShader;
    stage.stage               = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[STAGE_CLOSEST_HIT] = stage;

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
    group.closestHitShader = STAGE_CLOSEST_HIT;
    m_rtShaderGroups.push_back(group);

    // Pipeline layout
    VkPushConstantRange pc{};
    pc.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
    pc.offset     = 0;
    pc.size       = sizeof(RayTracingPushConstants);

    const std::vector descriptorLayouts{m_gpuGlobalUniformDataDescriptorLayout, m_bindlessDescriptorSetLayout};

    VkPipelineLayoutCreateInfo layout{};
    layout.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges    = &pc;
    layout.setLayoutCount         = static_cast<uint32_t>(descriptorLayouts.size());
    layout.pSetLayouts            = descriptorLayouts.data();

    vkCreatePipelineLayout(m_gfx.ctx, &layout, nullptr, &m_rayTracingPipeline.layout);

    // RT Pipeline
    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount                   = static_cast<uint32_t>(stages.size());// i.e. # of shaders
    pipelineInfo.pStages                      = stages.data();
    pipelineInfo.groupCount                   = static_cast<uint32_t>(m_rtShaderGroups.size());
    pipelineInfo.pGroups                      = m_rtShaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout                       = m_rayTracingPipeline.layout;
    vkCreateRayTracingPipelinesKHR(m_gfx.ctx, {}, {}, 1, &pipelineInfo, nullptr, &m_rayTracingPipeline.pipeline);

    vkDestroyShaderModule(m_gfx.ctx, raygenShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, missShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, closestHitShader, nullptr);

    LOG_DEBUG(VKE, "Ray tracing pipeline initialized");
}

void VkEngine::DestroyRayTracingPipeline() const {
    LOG_DEBUG(VKE, "Destroying ray tracing pipeline");

    m_rayTracingPipeline.Destroy(m_gfx.ctx, true);

    LOG_DEBUG(VKE, "Ray tracing pipeline destroyed");
}

inline uint32_t AlignUp(const uint32_t size, const uint32_t alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}

void VkEngine::InitRayTracingSBT() {
    LOG_DEBUG(VKE, "Initializing SBT");

    constexpr uint32_t rayGenCount = 1;
    constexpr uint32_t missCount   = 1;
    constexpr uint32_t hitCount    = 1;
    constexpr auto handleCount     = rayGenCount + missCount + hitCount;
    const uint32_t handleSize      = m_gfx.rtProperties.shaderGroupHandleSize;

    // TLDR: describing how to traverse the table for each type of shader
    // Stride is the size of the handle aligned to shaderGroupHandleAlignment (except ray gen)
    // Size of group is # of handles (aligned at shaderGroupHandleAlignment)
    const uint32_t handleSizeAligned = AlignUp(handleSize, m_gfx.rtProperties.shaderGroupHandleAlignment);

    m_SBT.rayGenRegion.stride = AlignUp(rayGenCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);
    m_SBT.rayGenRegion.size   = m_SBT.rayGenRegion.stride;

    m_SBT.missRegion.stride = handleSizeAligned;
    m_SBT.missRegion.size   = AlignUp(missCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);

    m_SBT.hitRegion.stride = handleSizeAligned;
    m_SBT.hitRegion.size   = AlignUp(hitCount * handleSizeAligned, m_gfx.rtProperties.shaderGroupBaseAlignment);

    // Shader group handles (where to access them in the pipeline)
    const uint32_t dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    CHECK_VK(vkGetRayTracingShaderGroupHandlesKHR(m_gfx.ctx, m_rayTracingPipeline.pipeline, 0, handleCount, dataSize, handles.data()));

    // Allocate buffer
    const VkDeviceSize sbtSize                    = m_SBT.rayGenRegion.size + m_SBT.missRegion.size + m_SBT.hitRegion.size + m_SBT.callableRegion.size;
    constexpr VkBufferUsageFlags bufferFlags      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    constexpr VmaMemoryUsage memUsage             = VMA_MEMORY_USAGE_CPU_TO_GPU;
    constexpr VmaAllocationCreateFlags allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    m_SBT.buffer                                  = m_gfx.CreateBuffer(sbtSize, bufferFlags, memUsage, allocFlags);

    const VkDeviceAddress sbtAddress = m_SBT.buffer.GetDeviceAddress(m_gfx.ctx);
    m_SBT.rayGenRegion.deviceAddress = sbtAddress;
    m_SBT.missRegion.deviceAddress   = sbtAddress + m_SBT.rayGenRegion.size;
    m_SBT.hitRegion.deviceAddress    = sbtAddress + m_SBT.rayGenRegion.size + m_SBT.missRegion.size;

    const auto GetHandle = [&](const int i) { return handles.data() + i * handleSize; };

    // Copy the handles retrieved from the pipeline into the buffer
    auto *pSbtBuffer   = static_cast<uint8_t *>(m_SBT.buffer.Map(m_gfx.allocator));
    uint8_t *pData     = nullptr;
    uint32_t handleIdx = 0;

    // Raygen
    pData = pSbtBuffer;
    memcpy(pData, GetHandle(handleIdx++), handleSize);

    pData = pSbtBuffer + m_SBT.rayGenRegion.size;
    for (uint32_t c = 0; c < missCount; ++c) {
        memcpy(pData, GetHandle(handleIdx++), handleSize);
        pData += m_SBT.missRegion.stride;
    }

    pData = pSbtBuffer + m_SBT.rayGenRegion.size + m_SBT.missRegion.size;
    for (uint32_t c = 0; c < hitCount; ++c) {
        memcpy(pData, GetHandle(handleIdx++), handleSize);
        pData += m_SBT.hitRegion.stride;
    }

    m_SBT.buffer.Unmap(m_gfx.allocator);

    LOG_DEBUG(VKE, "SBT initialized");
}

void VkEngine::DestroyRayTracingSBT() {
    LOG_DEBUG(VKE, "Destroying SBT");

    m_gfx.DestroyBuffer(m_SBT.buffer);

    LOG_DEBUG(VKE, "SBT destroyed");
}

void VkEngine::RayTrace(RenderContext &ctx, const glm::vec4 &clearColor) const {
    if (!m_bSceneLoaded) return;
    if (m_rtFrameNumber >= m_rtMaxFrames) return;

    const auto &drawImage = m_gfx.targets.draw32f;

    jvk::TransitionImageIfNeeded(ctx.cmd, drawImage.image, ctx.layout.draw32f, VK_IMAGE_LAYOUT_GENERAL);
    ctx.layout.draw32f = VK_IMAGE_LAYOUT_GENERAL;

    RayTracingPushConstants pc{};
    pc.invView         = glm::inverse(m_cache.view);
    pc.invProj         = glm::inverse(m_cache.proj);
    pc.frame           = m_rtFrameNumber;
    pc.samplesPerFrame = m_rtSamplesPerFrame;

    const auto sceneDescriptorSet = m_frameData[m_gfx.GetCurrentFrameIndex()].gpuGlobalUniformDataDescriptorSet;
    const std::vector descriptorSets{sceneDescriptorSet, m_bindlessDescriptorSet};
    vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rayTracingPipeline.pipeline);
    vkCmdBindDescriptorSets(
            ctx.cmd,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            m_rayTracingPipeline.layout,
            0,
            (uint32_t) descriptorSets.size(),
            descriptorSets.data(),
            0,
            nullptr);

    vkCmdPushConstants(
            ctx.cmd,
            m_rayTracingPipeline.layout,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
            0,
            sizeof(RayTracingPushConstants),
            &pc);
    vkCmdTraceRaysKHR(
            ctx.cmd,
            &m_SBT.rayGenRegion,
            &m_SBT.missRegion,
            &m_SBT.hitRegion,
            &m_SBT.callableRegion,
            m_viewRectangle.w,
            m_viewRectangle.h,
            1);
}

}// namespace jtx