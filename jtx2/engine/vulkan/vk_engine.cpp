#include <editor/editor.hpp>
#include <editor/ui_renderer.hpp>
#include <engine/vulkan/vk_engine.hpp>
#include <jvk/shaders.hpp>
#include <scene/scene.hpp>
#include <util/profiling.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <backends/imgui_impl_vulkan.h>
#include <engine/cpu/energy_compensation.hpp>
#include <engine/vulkan/accel.hpp>

#include <glm/gtx/transform.hpp>

namespace jtx {

void VkEngine::Init(const bool bEnableRayTracing) {
    TPROFILE_SCOPE();
    LOG_INFO(VKE, "Initializing Vulkan engine");
    m_bRayTracingAvailable = bEnableRayTracing;

    InitDescriptors();
    LoadLUTs();
    InitPipelines();
    if (m_bRayTracingAvailable) {
        InitRayTracingResources();
    }

    m_camera.settings.focalLength = 0.020f;
    m_camera.settings.sensorWidth = 0.036f;

    LOG_INFO(VKE, "Vulkan engine initialized");
}

void VkEngine::Destroy() {
    TPROFILE_SCOPE();
    LOG_INFO(VKE, "Destroying Vulkan engine");

    if (m_bRayTracingAvailable) {
        DestroyRayTracingResources();
    }

    DestroyScene();
    DestroyPipelines();
    DestroyLUTs();
    DestroyDescriptors();

    LOG_INFO(VKE, "Vulkan engine destroyed");
}

void VkEngine::RenderViewport(RenderContext &ctx, ResolveRegion &region, const SceneUpdate &update) {
    TPROFILE_SCOPE();
    // Reset frame count if ray tracing was just enabled
    m_vpState.bResetAccumulation |= m_bRayTracingEnabled && !m_bRayTracingEnabledPreviousFrame;
    m_bRayTracingEnabledPreviousFrame = m_bRayTracingEnabled;

    if (m_camera.HasChanged()) {
        m_camera.Update();

        m_vpState.bResetAccumulation = true;
    }

    UpdateGlobalUniformData();

    if (m_bSceneLoaded) {
        PopulateContext();
        if (UpdateScene(ctx, update)) {
            m_vpState.bResetAccumulation = true;
        }
        *m_frameData[ctx.frameIndex].gpuGlobalUniformDataMapping = m_gpuGlobalUniformData;
    }

    if (m_vpState.bResetAccumulation) {
        m_vpState.bResetAccumulation = false;
        m_vpState.currentSample      = 0;
    }

    // Calculate viewport
    const VkRect2D renderArea{
            {m_viewRectangle.x, m_viewRectangle.y},
            {m_viewRectangle.w, m_viewRectangle.h}};

    region.dst[0].width  = m_viewRectangle.x;
    region.dst[0].height = m_viewRectangle.y;
    region.dst[1].width  = m_viewRectangle.x + m_viewRectangle.w;
    region.dst[1].height = m_viewRectangle.y + m_viewRectangle.h;

    // TODO: rework ResolveRegion
    if (m_bRayTracingEnabled) {
        region.src[0]        = {0, 0};
        region.src[1].width  = m_viewRectangle.w;
        region.src[1].height = m_viewRectangle.h;
        region.target        = kRenderTarget::DRAW32f;

        RtRenderTargets targets{};
        targets.accumulation       = m_accumulationImage.image;
        targets.accumulationLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        targets.output             = m_gfx.targets.draw32f.image;
        targets.outputLayout       = ctx.layout.draw32f;

        m_vpState.invView = glm::inverse(m_cache.view);
        m_vpState.invProj = glm::inverse(m_cache.proj);
        m_vpState.width   = m_viewRectangle.w;
        m_vpState.height  = m_viewRectangle.h;

        RayTrace(ctx.cmd, m_vpSettings, m_vpState, targets);
        ctx.layout.draw32f = targets.outputLayout;
    } else {
        region.src[0] = region.dst[0];
        region.src[1] = region.dst[1];
        region.target = kRenderTarget::DRAW16f;
        Rasterize(ctx, renderArea, update.meshSelectionIndex);
    }
}

void VkEngine::PopulateContext() {
    TPROFILE_SCOPE();
    m_drawContext.objects.clear();

    // Loop through meshes
    for (uint32_t i = 0; i < m_pScene->meshes.size(); ++i) {
        const auto &mesh = m_pScene->meshes[i];
        GPURenderObject obj{};
        obj.objectID         = i;
        obj.start            = mesh.startIndex;
        obj.count            = mesh.numIndices;
        obj.materialPipeline = &m_rasterPipelines.diffuse;
        m_drawContext.objects.push_back(obj);
    }
}

void VkEngine::Rasterize(RenderContext &ctx, const VkRect2D &renderArea, const int32_t selectionIndex) {
    TPROFILE_SCOPE();
    // Begin render pass
    VkClearValue drawImageClearValue{};
    drawImageClearValue.color = {0.255f, 0.247f, 0.255f, 1.0f};

    const auto &drawImage    = m_gfx.targets.draw16f;
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

        // Bind layouts
        vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_rasterPipelines.layout, 0, 1, &frame.gpuGlobalUniformDataDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_rasterPipelines.layout, 1, 1, &m_bindlessDescriptorSet, 0, nullptr);

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
            vkCmdPushConstants(ctx.cmd, m_rasterPipelines.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

            // Need to multiply by 3 because r.count and r.start are relative to vec3u
            vkCmdDrawIndexed(ctx.cmd, r.count * 3, 1, r.start * 3, 0, 0);
        };

        for (const auto &r: opaqueDraws) {
            draw(m_drawContext.objects[r]);
        }

        if (selectionIndex > -1) {
            vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframePipeline.pipeline);
            vkCmdBindDescriptorSets(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframePipeline.layout, 0, 1, &frame.gpuGlobalUniformDataDescriptorSet, 0, nullptr);

            WireframePushConstants pc{};
            pc.wireframeColor = m_wireframeColor;
            vkCmdPushConstants(ctx.cmd, m_wireframePipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

            const auto &obj = m_drawContext.objects[selectionIndex];
            vkCmdDrawIndexed(ctx.cmd, obj.count * 3, 1, obj.start * 3, 0, 0);
        }
    }

    if (m_bDrawGrid) {
        vkCmdBindPipeline(ctx.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gridPipeline.pipeline);

        GridPushConstants pushConstants{};
        pushConstants.viewProj    = m_gpuGlobalUniformData.viewProj;
        pushConstants.cameraPos   = glm::vec4(m_gpuGlobalUniformData.cameraPosition, 0.0f);
        pushConstants.invViewProj = m_gpuGlobalUniformData.invViewProj;
        vkCmdPushConstants(ctx.cmd, m_gridPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(ctx.cmd, 3, 1, 0, 0);
    }

    vkCmdEndRenderingKHR(ctx.cmd);
}

void VkEngine::ProcessEvent(const SDL_Event &event) {
    TPROFILE_SCOPE();
    m_camera.ProcessSDLEvent(event);
}

void VkEngine::SkipEvent() {
    TPROFILE_SCOPE();
    m_camera.ResetInputState();
}

void VkEngine::DrawViewportSettingsPanel(UiDrawContext &ctx) {
    TPROFILE_SCOPE();
    ctx.StartRectangleBackground();

    if (ImGui::TreeNode("Viewport Camera")) {
        if (ImGui::TreeNode("FOV")) {
            if (ctx.StartTable("FOVTable")) {
                bool bCameraChanged = false;
                ctx.NewRow("Focal Length");
                bCameraChanged |= ImGui::DragFloat("##FocalLength", &m_camera.settings.focalLength, 0.01f, 0.01f, 100.0f);
                ctx.NewRow("Sensor Width");
                bCameraChanged |= ImGui::DragFloat("##SensorWidth", &m_camera.settings.sensorWidth, 0.01f, 0.01f, 100.0f);
                if (bCameraChanged) m_camera.NotifyChanged();
                ctx.EndTable();
            }
            ImGui::TreePop();
        }
        // These don't affect rendering--dont need to notify
        if (ImGui::TreeNode("Control Sensitivity")) {
            if (ctx.StartTable("SensitivityTable")) {
                ctx.NewRow("Zoom");
                ImGui::DragFloat("##ZoomSensitivity", &m_camera.dollySpeed, 0.01f, 0.01f, 100.0f);
                ctx.NewRow("Orbit");
                ImGui::DragFloat("##OrbitSensitivity", &m_camera.orbitSpeed, 0.01f, 0.01f, 100.0f);
                ctx.NewRow("Pan");
                ImGui::DragFloat("##PanSensitivity", &m_camera.panSpeed, 0.01f, 0.01f, 100.0f);
                ctx.EndTable();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Scene Controls")) {
            // TODO: Improve UX here, just need something to work for now
            if (ImGui::Button("Camera View", ctx.GetAvailWidth())) {
                if (m_pScene != nullptr) {
                    const auto &cam     = m_pScene->camera;
                    const glm::vec3 pos = {cam.position.x, cam.position.y, cam.position.z};
                    const glm::vec3 tgt = {cam.target.x, cam.target.y, cam.target.z};
                    const glm::vec3 up  = {cam.up.x, cam.up.y, cam.up.z};
                    m_camera.SetView(pos, tgt, up);

                    // TODO: incorporate the rest of the lens settings once they're implemented on the GPU
                    m_camera.settings.focalLength = cam.settings.focalLength;
                    m_camera.settings.sensorWidth = cam.settings.sensorWidth;
                }
            }

            if (ImGui::Button("Sync Scene Camera", ctx.GetAvailWidth())) {
                if (m_pScene != nullptr) {
                    auto &cam     = m_pScene->camera;
                    cam.position  = vec3(&m_camera.position.x);
                    cam.target    = vec3(&m_camera.target.x);
                    const auto up = m_camera.GetUpVector();
                    cam.up        = vec3(&up.x);

                    // TODO: incorporate the rest of the lens settings once they're implemented on the GPU
                    cam.settings.focalLength = m_camera.settings.focalLength;
                    cam.settings.sensorWidth = m_camera.settings.sensorWidth;
                }
            }

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Rasterization")) {
        if (ctx.StartTable("VkRasterizationTable")) {
            ctx.NewRow("Draw grid");
            ImGui::Checkbox("##Grid", &m_bDrawGrid);
            ctx.NewRow("Near Clip");
            ImGui::DragFloat("##NearClip", &m_nearClip, 0.01f, 0.001f, 100.0f);
            ctx.NewRow("Far Clip");
            ImGui::DragFloat("##FarClip", &m_farClip, 1.0f, 1.0f, 1000000.0f);
            ctx.NewRow("Wireframe Color");
            ImGui::ColorEdit4("##WireframeColor", &m_wireframeColor.x);
            ctx.EndTable();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Path Tracing")) {
        if (ctx.StartTable("VkRayTracingTable")) {
            if (!m_bRayTracingAvailable) ImGui::BeginDisabled();
            ctx.NewRow("Enable");
            ImGui::Checkbox("##RT", &m_bRayTracingEnabled);

            if (!m_bRayTracingEnabled && m_bRayTracingAvailable) ImGui::BeginDisabled();
            ctx.NewRow("Samples Per Pixel");
            int32_t spp = m_vpSettings.targetSamples;
            if (ImGui::DragInt("##SamplesPerPixel", &spp)) {
                m_vpState.bResetAccumulation = true;
                m_vpSettings.targetSamples   = static_cast<uint32_t>(spp);
            }

            ctx.NewRow("Samples Per Frame");

            int32_t samplePerFrame = m_vpSettings.samplesPerFrame;
            if (ImGui::DragInt("##SamplesPerFrame", &samplePerFrame, 1, 1, 32)) {
                m_vpState.bResetAccumulation = true;
                m_vpSettings.samplesPerFrame = static_cast<uint32_t>(samplePerFrame);
            }
            ctx.EndTable();

            if (ImGui::TreeNode("Post Processing")) {
                if (ctx.StartTable("PostProcessingTable")) {
                    ctx.NewRow("Exposure");
                    if (ImGui::InputFloat("##Exposure", &m_vpSettings.postProcessing.EC, 1)) {
                        m_vpState.bPostProcessSettingsChanged = true;
                    }

                    const char *tmo[]      = {"None", "Reinhard", "ACES", "AgX", "Hable Filmic"};
                    static int selectedTmo = m_vpSettings.postProcessing.tonemappingOp;
                    ctx.NewRow("Tonemapping");
                    if (ImGui::Combo("##TMO", &selectedTmo, tmo, IM_ARRAYSIZE(tmo))) {
                        m_vpSettings.postProcessing.tonemappingOp = selectedTmo;
                        m_vpState.bPostProcessSettingsChanged     = true;
                    }
                    ctx.EndTable();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Clamping")) {
                if (ctx.StartTable("Clamping")) {
                    ctx.NewRow("Direct Lighting");
                    m_vpState.bResetAccumulation |= ImGui::DragFloat("##DirectLighting", &m_vpSettings.directClamping, 0.1f, 0.0f, 100.0f);
                    ctx.NewRow("Indirect Lighting");
                    m_vpState.bResetAccumulation |= ImGui::DragFloat("##IndirectLighting", &m_vpSettings.indirectClamping, 0.1f, 0.0f, 100.0f);
                    ctx.EndTable();
                }

                ImGui::TreePop();
            }

            if (!m_bRayTracingAvailable || !m_bRayTracingEnabled) ImGui::EndDisabled();
        }
        ImGui::TreePop();
    }

    ctx.EndRectangleBackground(true);
}

bool VkEngine::DrawRenderPanel(UiDrawContext &ctx) {
    TPROFILE_SCOPE();
    ImGui::Text("Render Progress:");
    const float progress = static_cast<float>(m_renderState.currentSample) / static_cast<float>(m_renderSettings.targetSamples);
    ImGui::ProgressBar(progress);

    if (!m_renderState.bRenderDone) {
        const auto now = std::chrono::high_resolution_clock::now();
        m_elapsedTime  = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - m_renderStartTime).count();
    }
    ImGui::Text("Time Elapsed: %f s", m_elapsedTime / 1000.0);

    ctx.InsertPadding();

    if (ImGui::CollapsingHeader("Post Processing")) {
        ctx.StartRectangleBackground();
        if (ctx.StartTable("RenderPostProcessingTable")) {
            ctx.NewRow("Exposure");
            if (ImGui::InputFloat("##Exposure", &m_renderSettings.postProcessing.EC, 1)) {
                m_renderState.bPostProcessSettingsChanged = true;
            }

            const char *tmo[]      = {"None", "Reinhard", "ACES", "AgX", "Hable Filmic"};
            static int selectedTmo = m_renderSettings.postProcessing.tonemappingOp;
            ctx.NewRow("Tonemapping");
            if (ImGui::Combo("##TMO", &selectedTmo, tmo, IM_ARRAYSIZE(tmo))) {
                m_renderSettings.postProcessing.tonemappingOp = selectedTmo;
                m_renderState.bPostProcessSettingsChanged     = true;
            }
            ctx.EndTable();
        }
        ctx.EndRectangleBackground(true);
    }

    return m_renderState.bRenderDone;
}

void VkEngine::LoadHDRI() {
    TPROFILE_SCOPE();
    if (m_gpuSceneData.envmapIndex >= 0) {
        m_gfx.DestroyImage(m_gpuSceneData.textures[m_gpuSceneData.envmapIndex]);
        LOG_DEBUG(VKE, "Destroyed previously loaded HDRI texture");
    }

    if (!m_pScene->envmap.image.IsEmpty()) {
        LOG_DEBUG(VKE, "Loading HDRI envmap");
        const auto &tex = m_pScene->envmap.image;

        int32_t index;
        bool bPushBack = false;
        if (m_gpuSceneData.envmapIndex >= 0) {
            index = m_gpuSceneData.envmapIndex;
        } else {
            index     = m_gpuSceneData.textures.size();
            bPushBack = true;
        }

        constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        const VkExtent3D extent   = {static_cast<uint32_t>(tex.width), static_cast<uint32_t>(tex.height), 1};
        const jvk::Image gpuTex   = m_gfx.CreateImage(tex.pData, extent, tex.channels, format, VK_IMAGE_USAGE_SAMPLED_BIT, 4);

        jvk::DescriptorWriter writer;
        writer.WriteImage(
                kL2Bindings::GPU_TEXTURE_SAMPLER_ARRAY,
                index,
                gpuTex.view,
                m_gfx.defaultSamplers.linear,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.UpdateSet(m_gfx.ctx, m_bindlessDescriptorSet);

        m_gpuSceneData.envmapIndex = index;
        if (bPushBack) m_gpuSceneData.textures.push_back(gpuTex);
        else
            m_gpuSceneData.textures[index] = gpuTex;

        LOG_DEBUG(VKE, "HDRI envmap loaded");
    } else {
        m_gpuSceneData.envmapIndex = -1;
        LOG_DEBUG(VKE, "No HDRI texture found");
    }
}

VkDescriptorSet VkEngine::InitRenderResources(const RenderSettings &rs) {
    TPROFILE_SCOPE();
    // TODO: just give this its own descriptor set
    m_gfx.WaitIdle();

    m_renderSettings                  = {};
    m_renderSettings.targetSamples    = rs.spp;
    m_renderSettings.samplesPerFrame  = rs.samplesPerPass;
    m_renderSettings.directClamping   = rs.directClamping;
    m_renderSettings.indirectClamping = rs.indirectClamping;

    m_renderSettings.postProcessing.exposureType  = rs.exposureMode;
    m_renderSettings.postProcessing.EC            = rs.EC;
    m_renderSettings.postProcessing.EV            = rs.EV;
    m_renderSettings.postProcessing.tonemappingOp = rs.tonemapOp;

    m_renderState = {};

    // TODO: cursed...change when GPU implements full thin lens model
    auto &cam     = m_pScene->camera;
    glm::vec3 pos = {cam.position.x, cam.position.y, cam.position.z};
    glm::vec3 tgt = {cam.target.x, cam.target.y, cam.target.z};
    glm::vec3 up  = {cam.up.x, cam.up.y, cam.up.z};
    OrbitCamera ocam{pos, tgt, up};

    float aspectRatio     = static_cast<float>(rs.width) / static_cast<float>(rs.height);
    m_renderState.invProj = glm::inverse(ocam.GetProjectionMatrix(aspectRatio, m_nearClip, m_farClip));
    m_renderState.invView = glm::inverse(ocam.GetViewMatrix());

    m_renderState.width  = rs.width;
    m_renderState.height = rs.height;

    jvk::Image &accImage = m_renderResources.accumulationImage;
    jvk::Image &outImage = m_renderResources.outputImage;

    VkExtent3D extent{};
    extent.width  = rs.width;
    extent.height = rs.height;
    extent.depth  = 1;

    accImage.extent = extent;
    outImage.extent = extent;

    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

    VkImageUsageFlags usages{};
    usages |= VK_IMAGE_USAGE_STORAGE_BIT;

    // Accumulation image
    VkImageCreateInfo imageInfo = jvk::init::Image(format, usages, extent);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(m_gfx.allocator, &imageInfo, &allocInfo, &accImage.image, &accImage.allocation, nullptr);

    VkImageViewCreateInfo viewInfo = jvk::init::ImageView(format, accImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(m_gfx.ctx, &viewInfo, nullptr, &accImage.view));

    // Output image
    usages |= VK_IMAGE_USAGE_SAMPLED_BIT;     // Progressive render for UI
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;// Saving to CPU buffer

    imageInfo = jvk::init::Image(format, usages, extent);
    vmaCreateImage(m_gfx.allocator, &imageInfo, &allocInfo, &outImage.image, &outImage.allocation, nullptr);

    viewInfo = jvk::init::ImageView(format, outImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(m_gfx.ctx, &viewInfo, nullptr, &outImage.view));

    // Write these images to the descriptor set
    jvk::DescriptorWriter writer;
    writer.WriteImage(1, accImage.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.WriteImage(2, outImage.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    for (const auto &frame: m_frameData) {
        writer.UpdateSet(m_gfx.ctx, frame.gpuGlobalUniformDataDescriptorSet);
    }

    m_renderTargets.accumulation       = m_renderResources.accumulationImage.image;
    m_renderTargets.accumulationLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_renderTargets.output             = m_renderResources.outputImage.image;
    m_renderTargets.outputLayout       = VK_IMAGE_LAYOUT_UNDEFINED;

    // UI texture descriptor set
    m_renderResources.outputDescriptorSet = ImGui_ImplVulkan_AddTexture(m_gfx.defaultSamplers.linear, outImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_renderStartTime = std::chrono::high_resolution_clock::now();
    return m_renderResources.outputDescriptorSet;
}

void VkEngine::AdvanceRender(const VkCommandBuffer cmd) {
    TPROFILE_SCOPE();

    if (RayTrace(cmd, m_renderSettings, m_renderState, m_renderTargets)) {
        // (Wireframe pipeline is never enabled for final/offline -- we can hardcode this barrier)
        VkImageMemoryBarrier2 barrier{};
        barrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.srcAccessMask    = VK_ACCESS_2_SHADER_WRITE_BIT;
        barrier.dstStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask    = VK_ACCESS_2_SHADER_READ_BIT;
        barrier.oldLayout        = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image            = m_renderTargets.output;
        barrier.subresourceRange = jvk::init::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        VkDependencyInfo dependency{};
        dependency.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2KHR(cmd, &dependency);
        m_renderTargets.outputLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

void VkEngine::SaveRenderImage(const std::filesystem::path &path) const {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Saving render image");

    const auto &img = m_renderResources.outputImage;
    const VkExtent2D extent{img.extent.width, img.extent.height};

    VkImage hostImage;
    VmaAllocation hostAlloc;
    VkImageCreateInfo imageInfo = jvk::init::Image(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT, img.extent, VK_SAMPLE_COUNT_1_BIT);
    imageInfo.tiling            = VK_IMAGE_TILING_LINEAR;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_TO_CPU;

    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CHECK_VK(vmaCreateImage(m_gfx.allocator, &imageInfo, &allocInfo, &hostImage, &hostAlloc, nullptr));

    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        jvk::TransitionImage(cmd, img.image, m_renderTargets.outputLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        jvk::TransitionImage(cmd, hostImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        jvk::CopyImageToImage(cmd, img.image, hostImage, extent, extent);

        jvk::TransitionImage(cmd, hostImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);// Required to map the image
        jvk::TransitionImage(cmd, img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_renderTargets.outputLayout);
    });

    VkImageSubresource subresource{};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subresourceLayout{};
    vkGetImageSubresourceLayout(m_gfx.ctx, hostImage, &subresource, &subresourceLayout);

    uint8_t *pData;
    vmaMapMemory(m_gfx.allocator, hostAlloc, (void **) &pData);
    pData += subresourceLayout.offset;

    const auto result = Image8u::Save(pData, extent.width, extent.height, 4, path);
    if (result > 0) {
        LOG_DEBUG(VKE, "Render image saved to: {}", path.string().c_str());
    } else {
        LOG_ERROR(VKE, "Failed to save render image: {}", string_JtxResult(result));
    }

    vmaUnmapMemory(m_gfx.allocator, hostAlloc);
    vmaDestroyImage(m_gfx.allocator, hostImage, hostAlloc);
}

void VkEngine::DestroyRenderResources() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying render resources");

    // TODO: remove this
    m_gfx.WaitIdle();

    ImGui_ImplVulkan_RemoveTexture(m_renderResources.outputDescriptorSet);

    // Revert changes to uniform data
    jvk::DescriptorWriter writer;
    writer.WriteImage(1, m_accumulationImage.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.WriteImage(2, m_gfx.targets.draw32f.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    for (const auto &frame: m_frameData) {
        writer.UpdateSet(m_gfx.ctx, frame.gpuGlobalUniformDataDescriptorSet);
    }

    m_gfx.DestroyImage(m_renderResources.accumulationImage);
    m_gfx.DestroyImage(m_renderResources.outputImage);

    LOG_DEBUG(VKE, "Render resources destroyed");
}

void VkEngine::InitDescriptors() {
    TPROFILE_SCOPE();
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
        shaderStages |= VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    }

    jvk::DescriptorLayoutBuilder builder;
    builder.AddBinding(kL2Bindings::GPU_OBJECT_DATA, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStages);
    builder.AddBinding(kL2Bindings::GPU_MATERIAL_DATA, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStages);
    builder.AddBinding(kL2Bindings::GPU_TEXTURE_SAMPLER_ARRAY, 256, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStages);
    if (m_bRayTracingAvailable) {
        // Should NOT be available during rasterization
        builder.AddBinding(kL2Bindings::GPU_TLAS, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    }

    constexpr VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    std::vector vBindingFlags                       = {
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
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}};
    m_descriptorAllocator.InitPool(m_gfx.ctx, 2, poolSizesGlobal);

    builder.Clear();

    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStages);
    // Raygen needs access to the draw image
    if (m_bRayTracingAvailable) {
        builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
        builder.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_gpuGlobalUniformDataDescriptorLayout = builder.Build(m_gfx.ctx);

    for (auto &frame: m_frameData) {
        frame.gpuGlobalUniformDataDescriptorSet = m_descriptorAllocator.Allocate(m_gfx.ctx, m_gpuGlobalUniformDataDescriptorLayout);
    }
}

void VkEngine::DestroyDescriptors() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying descriptors");

    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_bindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_gfx.ctx, m_gpuGlobalUniformDataDescriptorLayout, nullptr);

    m_bindlessAllocator.DestroyPool(m_gfx.ctx);
    m_descriptorAllocator.DestroyPool(m_gfx.ctx);

    LOG_DEBUG(VKE, "Descriptors destroyed");
}

void VkEngine::UpdateGlobalUniformData() {
    TPROFILE_SCOPE();
    float aspectRatio;
    if (m_viewRectangle.w > 0 && m_viewRectangle.h > 0) {
        aspectRatio = static_cast<float>(m_viewRectangle.w) / static_cast<float>(m_viewRectangle.h);
    } else {
        aspectRatio = static_cast<float>(m_gfx.window.extent.width) / static_cast<float>(m_gfx.window.extent.width);
    }

    m_cache.view = m_camera.GetViewMatrix();
    m_cache.proj = m_camera.GetProjectionMatrix(aspectRatio, m_nearClip, m_farClip);
    m_cache.proj[1][1] *= -1;

    m_gpuGlobalUniformData.viewProj       = m_cache.proj * m_cache.view;
    m_gpuGlobalUniformData.invViewProj    = glm::inverse(m_gpuGlobalUniformData.viewProj);
    m_gpuGlobalUniformData.cameraPosition = glm::vec4(m_camera.position, 0.0f);

    if (m_pScene) {
        const auto &envmap                      = m_pScene->envmap;
        m_gpuGlobalUniformData.envmapType       = envmap.type;
        m_gpuGlobalUniformData.envmapTexture    = m_gpuSceneData.envmapIndex;
        m_gpuGlobalUniformData.envmapColor      = glm::vec3(envmap.uniform[0], envmap.uniform[1], envmap.uniform[2]);
        m_gpuGlobalUniformData.envmapIntensity  = envmap.intensity;
        m_gpuGlobalUniformData.horizontalOffset = envmap.horizontalOffset;
        m_gpuGlobalUniformData.verticalOffset   = envmap.verticalOffset;
    }

    m_gpuGlobalUniformData.vertexBuffer   = m_gpuSceneData.positionAddress;
    m_gpuGlobalUniformData.normalBuffer   = m_gpuSceneData.normalAddress;
    m_gpuGlobalUniformData.texCoordBuffer = m_gpuSceneData.uvAddress;
    m_gpuGlobalUniformData.colorBuffer    = m_gpuSceneData.colorAddress;
    m_gpuGlobalUniformData.indexBuffer    = m_gpuSceneData.indexAddress;
}

void VkEngine::LoadLUTs() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Loading LUTs");

    // The default samplers won't work here since we need VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    VkSamplerCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.minFilter    = VK_FILTER_NEAREST;
    info.magFilter    = VK_FILTER_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    CHECK_VK(vkCreateSampler(m_gfx.ctx, &info, nullptr, &m_lutSampler));

    Image32f ggxReflection;
    CHECK_JTX(Image32f::LoadLUT(GGX_COMPENSATION_LUT_WIDTH, GGX_COMPENSATION_LUT_HEIGHT, 1, "lut/ggx.lut", ggxReflection));

    const VkFormat format   = VK_FORMAT_R32_SFLOAT;
    const VkExtent3D extent = {GGX_COMPENSATION_LUT_WIDTH, GGX_COMPENSATION_LUT_HEIGHT, 1};

    m_luts.ggxReflection = m_gfx.CreateImage(ggxReflection.pData, extent, 1, format, VK_IMAGE_USAGE_SAMPLED_BIT, sizeof(float));

    LOG_DEBUG(VKE, "LUTs loaded");
}

void VkEngine::DestroyLUTs() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying LUTs");

    m_luts.ggxReflection.Destroy(m_gfx.ctx, m_gfx.allocator);
    vkDestroySampler(m_gfx.ctx, m_lutSampler, nullptr);

    LOG_DEBUG(VKE, "LUTs destroyed");
}

void VkEngine::InitPipelines() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing Pipelines");

    InitRasterPipelines();
    InitGridPipeline();
    if (m_bRayTracingAvailable) {
        InitRayTracingPipeline();
        InitRTPostProcessingPipeline();
    }

    LOG_DEBUG(VKE, "Pipelines initialized");
}

void VkEngine::DestroyPipelines() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying Pipelines");

    if (m_bRayTracingAvailable) {
        DestroyRayTracingPipeline();
        DestroyRTPostProcessingPipeline();
    }
    DestroyGridPipeline();
    DestroyRasterPipelines();

    LOG_DEBUG(VKE, "Pipelines destroyed");
}

void VkEngine::InitRasterPipelines() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing rasterization pipelines");
    VkShaderModule vertexShader;
    if (!jvk::LoadShaderModule("spv/mesh_vertexMain.spv", m_gfx.ctx, &vertexShader)) {
        LOG_FATAL(VKE, "Failed to load mesh vertex shader");
    }

    VkShaderModule fragmentShader;
    if (!jvk::LoadShaderModule("spv/mesh_fragmentMain.spv", m_gfx.ctx, &fragmentShader)) {
        LOG_FATAL(VKE, "Failed to load mesh fragment shader");
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
    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_rasterPipelines.layout));

    jvk::PipelineBuilder builder;
    builder.SetShaders(vertexShader, fragmentShader);
    builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.SetMultiSamplingNone();
    builder.DisableBlending();
    builder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.DisableStencilTest();
    builder.SetColorAttachmentFormat(m_gfx.targets.draw16f.format);
    builder.SetDepthAttachmentFormat(m_gfx.targets.depthStencil.format);
    builder.pipelineLayout = m_rasterPipelines.layout;

    m_rasterPipelines.diffuse = builder.BuildPipeline(m_gfx.ctx);

    // Create more pipelines as needed here
    vkDestroyShaderModule(m_gfx.ctx, vertexShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, fragmentShader, nullptr);

    // Wireframe
    LOG_DEBUG(VKE, "Initializing wireframe pipeline");

    pc.offset     = 0;
    pc.size       = sizeof(WireframePushConstants);
    pc.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    layoutInfo.setLayoutCount             = 1;
    layoutInfo.pSetLayouts                = &m_gpuGlobalUniformDataDescriptorLayout;
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pc;
    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_wireframePipeline.layout));

    if (!jvk::LoadShaderModule("spv/wireframe_vertexMain.spv", m_gfx.ctx, &vertexShader)) {
        LOG_FATAL(VKE, "Failed to load wireframe vertex shader");
    }

    if (!jvk::LoadShaderModule("spv/wireframe_fragmentMain.spv", m_gfx.ctx, &fragmentShader)) {
        LOG_FATAL(VKE, "Failed to load wireframe fragment shader");
    }

    builder.pipelineLayout = m_wireframePipeline.layout;
    builder.SetShaders(vertexShader, fragmentShader);
    builder.SetPolygonMode(VK_POLYGON_MODE_LINE);
    builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.EnableBlendingAlphaBlend();
    builder.DisableDepthTest();
    m_wireframePipeline.pipeline = builder.BuildPipeline(m_gfx.ctx);

    vkDestroyShaderModule(m_gfx.ctx, vertexShader, nullptr);
    vkDestroyShaderModule(m_gfx.ctx, fragmentShader, nullptr);

    LOG_DEBUG(VKE, "Rasterization pipelines initialized");
}

void VkEngine::DestroyRasterPipelines() const {
    TPROFILE_SCOPE();
    m_wireframePipeline.Destroy(m_gfx.ctx, true);
    vkDestroyPipelineLayout(m_gfx.ctx, m_rasterPipelines.layout, nullptr);
    vkDestroyPipeline(m_gfx.ctx, m_rasterPipelines.diffuse, nullptr);
}

void VkEngine::InitGridPipeline() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing grid pipeline");
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
    LOG_DEBUG(VKE, "Grid pipeline initialized");
}

void VkEngine::DestroyGridPipeline() const {
    TPROFILE_SCOPE();
    m_gridPipeline.Destroy(m_gfx.ctx, true);
}

void VkEngine::LoadScene(Scene *pScene) {
    TPROFILE_SCOPE();
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
                writer.WriteImage(1, m_accumulationImage.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
                writer.WriteImage(2, m_gfx.targets.draw32f.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            }
            writer.UpdateSet(m_gfx.ctx, frame.gpuGlobalUniformDataDescriptorSet);
            writer.Clear();

            // Staging buffers for live material updates
            frame.materialStagingBuffer = m_gfx.CreateBuffer(
                    sizeof(GPUMaterialData),
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            frame.objectStagingBuffer = m_gfx.CreateBuffer(
                    sizeof(GPUObjectData),
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
        *frame.gpuGlobalUniformDataMapping = {};
    }
    LOG_DEBUG(VKE, "Global uniform data buffers prepared");

    // -- Textures --
    LOG_DEBUG(VKE, "Loading textures");

    // Load LUTs into first few slots
    // Expand GPU texture array even though theyre stored separately to keep indices accurate
    // TODO: Setup proper indexing between LUTs and scene textures
    //       Maybe put LUTs at the end of the texture array?
    m_gpuSceneData.textures.resize(pScene->textures.size() + m_numLuts);

    uint32_t index = 0;
    writer.WriteImage(kL2Bindings::GPU_TEXTURE_SAMPLER_ARRAY,
                      index++,
                      m_luts.ggxReflection.view,
                      m_gfx.defaultSamplers.nearest,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    LOG_DEBUG(VKE, "    Scene has {} textures", pScene->textures.size());
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
                m_gfx.defaultSamplers.nearest,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        m_gpuSceneData.textures[index++] = gpuTex;
    }

    LOG_DEBUG(VKE, "Textures loaded");

    m_gpuSceneData.envmapIndex = -1;// Reset to avoid double-delete
    LoadHDRI();

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

    const auto materialBufferSize = sizeof(GPUMaterialData) * pScene->materials.size();
    m_gpuSceneData.materialBuffer = m_gfx.CreateBuffer(materialBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    staging                       = m_gfx.CreateBuffer(materialBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    LOG_DEBUG(VKE, "    Created material buffer of size {} bytes", sizeof(GPUMaterialData) * pScene->materials.size());

    void *materialData = staging.Map(m_gfx.allocator);
    offset             = 0;
    for (const auto &material: pScene->materials) {
        GPUMaterialData m{};
        m.diffuse           = vec4(material.parameters.diffuse, 0.0f);
        m.ior               = vec4(material.parameters.ior, 0.0f);
        m.f0                = vec4(material.parameters.f0, 0.0f);
        m.emission          = vec4(material.parameters.emission, 0.0f);
        m.transmissionColor = vec4(material.parameters.transmissionColor, 0.0f);
        m.emissionStrength  = material.parameters.emissionStrength;
        m.roughness         = material.parameters.roughness;
        m.anisotropy        = material.parameters.anisotropy;
        m.diffuseRoughness  = material.parameters.diffuseRoughness;
        m.specularTint      = material.parameters.specularTint;
        m.baseColorTexture  = material.textureIndices.baseColor < 0 ? -1 : material.textureIndices.baseColor + m_numLuts;
        m.type              = material.mType;

        static_cast<GPUMaterialData *>(materialData)[offset++] = m;
    }

    LOG_DEBUG(VKE, "    Material data copied to staging buffer");

    VkBufferCopy copyRegion{};
    copyRegion.dstOffset = 0;
    copyRegion.srcOffset = 0;
    copyRegion.size      = materialBufferSize;

    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
        vkCmdCopyBuffer(cmd, staging.buffer, m_gpuSceneData.materialBuffer.buffer, 1, &copyRegion);
    });

    staging.Unmap(m_gfx.allocator);
    m_gfx.DestroyBuffer(staging);

    LOG_DEBUG(VKE, "    Material data copied to GPU buffer");

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
        obj.material   = mesh.materialIndex;
        gpuObjects.push_back(obj);

        GPURenderObject rObj{};
        rObj.start = mesh.startIndex;
        rObj.count = mesh.numIndices;
        // TODO: assign this dynamically
        rObj.materialPipeline = &m_rasterPipelines.diffuse;
    }
    size_t objSize              = sizeof(GPUObjectData) * gpuObjects.size();
    m_gpuSceneData.objectBuffer = m_gfx.CreateBuffer(objSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    LOG_DEBUG(VKE, "   Created GPU objects buffer of size {} bytes", objSize);

    staging = m_gfx.CreateBuffer(objSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    LOG_DEBUG(VKE, "   Created staging buffer  of size {} bytes", objSize);
    memcpy(staging.GetMapping(), gpuObjects.data(), objSize);
    LOG_DEBUG(VKE, "   Copied objects to staging buffer");


    m_gfx.imBuffer.SubmitAndWait(m_gfx.graphicsQueue, [&](const VkCommandBuffer cmd) {
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
    TPROFILE_SCOPE();
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
        for (int i = m_numLuts; i < m_gpuSceneData.textures.size(); i++) {
            m_gfx.DestroyImage(m_gpuSceneData.textures[i]);
        }

        m_gpuSceneData.textures.clear();
        LOG_DEBUG(VKE, "Destroyed textures");

        // -- Global data --
        for (auto &frame: m_frameData) {
            frame.gpuGlobalUniformData.Unmap(m_gfx.allocator);
            m_gfx.DestroyBuffer(frame.gpuGlobalUniformData);
            m_gfx.DestroyBuffer(frame.objectStagingBuffer);
            m_gfx.DestroyBuffer(frame.materialStagingBuffer);
        }
        LOG_DEBUG(VKE, "Destroyed global uniform data buffers");

        m_pScene = nullptr;
        LOG_INFO(VKE, "Scene destroyed");
    }
    m_bSceneLoaded = false;
}

bool VkEngine::UpdateScene(const RenderContext &ctx, const SceneUpdate &update) const {
    TPROFILE_SCOPE();
    bool bUpdated = update.bResetAccumulation;

    if (update.materialIndex > -1) {
        bUpdated          = true;
        const auto &frame = m_frameData[ctx.frameIndex];

        const auto &material   = m_pScene->materials[update.materialIndex];
        const auto data        = static_cast<GPUMaterialData *>(frame.materialStagingBuffer.GetMapping());
        data->diffuse           = vec4(material.parameters.diffuse, 0.0f);
        data->ior               = vec4(material.parameters.ior, 0.0f);
        data->f0                = vec4(material.parameters.f0, 0.0f);
        data->emission          = vec4(material.parameters.emission, 0.0f);
        data->transmissionColor = vec4(material.parameters.transmissionColor, 0.0f);
        data->emissionStrength  = material.parameters.emissionStrength;
        data->roughness         = material.parameters.roughness;
        data->anisotropy        = material.parameters.anisotropy;
        data->diffuseRoughness  = material.parameters.diffuseRoughness;
        data->specularTint      = material.parameters.specularTint;
        data->baseColorTexture  = material.textureIndices.baseColor < 0 ? -1 : material.textureIndices.baseColor + m_numLuts;
        data->type              = material.mType;

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = sizeof(GPUMaterialData) * update.materialIndex;
        copyRegion.size      = sizeof(GPUMaterialData);

        vkCmdCopyBuffer(ctx.cmd, frame.materialStagingBuffer.buffer, m_gpuSceneData.materialBuffer.buffer, 1, &copyRegion);

        // Make sure this copy finishes before anything draws
        VkMemoryBarrier2 barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        if (m_bRayTracingEnabled) {
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        } else {
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

        VkDependencyInfo dep{};
        dep.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2KHR(ctx.cmd, &dep);
    }

    if (update.objectIndex > -1) {
        bUpdated = true;

        const auto &frame = m_frameData[ctx.frameIndex];
        const auto &obj   = m_pScene->meshes[update.objectIndex];
        const auto data   = static_cast<GPUObjectData *>(frame.objectStagingBuffer.GetMapping());
        data->world       = glm::mat4(1.0f);
        data->normal      = glm::mat4(1.0f);
        data->startIndex  = obj.startIndex;
        data->material    = obj.materialIndex;

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = sizeof(GPUObjectData) * update.objectIndex;
        copyRegion.size      = sizeof(GPUObjectData);

        vkCmdCopyBuffer(ctx.cmd, frame.objectStagingBuffer.buffer, m_gpuSceneData.objectBuffer.buffer, 1, &copyRegion);

        VkMemoryBarrier2 barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        if (m_bRayTracingEnabled) {
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        } else {
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
        }
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

        VkDependencyInfo dep{};
        dep.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2KHR(ctx.cmd, &dep);
    }

    return bUpdated;
}

void VkEngine::BuildBLAS() {
    TPROFILE_SCOPE();
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
    TPROFILE_SCOPE();
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
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing ray tracing pipeline");

    VkShaderModule raygenShader;
    if (!jvk::LoadShaderModule("spv/raytrace_rayGenMain.spv", m_gfx.ctx, &raygenShader)) {
        LOG_FATAL(VKE, "Failed to load raygen shader");
    }

    VkShaderModule missShader;
    if (!jvk::LoadShaderModule("spv/raytrace_missMain.spv", m_gfx.ctx, &missShader)) {
        LOG_FATAL(VKE, "Failed to load miss shader");
    }

    VkShaderModule closestHitShader;
    if (!jvk::LoadShaderModule("spv/raytrace_closestHitMain.spv", m_gfx.ctx, &closestHitShader)) {
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
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying ray tracing pipeline");

    m_rayTracingPipeline.Destroy(m_gfx.ctx, true);

    LOG_DEBUG(VKE, "Ray tracing pipeline destroyed");
}

void VkEngine::InitRTPostProcessingPipeline() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing ray tracing post processing pipeline");
    VkShaderModule computeShader;
    if (!jvk::LoadShaderModule("spv/postprocessing_computeMain.spv", m_gfx.ctx, &computeShader)) {
        LOG_FATAL(VKE, "Failed to load post-processing compute shader");
    }

    VkPushConstantRange pc{};
    pc.offset     = 0;
    pc.size       = sizeof(PostProcessingPushConstants);
    pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::vector descriptorLayouts{m_gpuGlobalUniformDataDescriptorLayout, m_bindlessDescriptorSetLayout};

    VkPipelineLayoutCreateInfo layoutInfo = jvk::init::PipelineLayout();
    layoutInfo.pushConstantRangeCount     = 1;
    layoutInfo.pPushConstantRanges        = &pc;
    layoutInfo.setLayoutCount             = static_cast<uint32_t>(descriptorLayouts.size());
    layoutInfo.pSetLayouts                = descriptorLayouts.data();
    CHECK_VK(vkCreatePipelineLayout(m_gfx.ctx, &layoutInfo, nullptr, &m_rtPostProcessingPipeline.layout));

    const VkPipelineShaderStageCreateInfo stage = jvk::init::PipelineShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext  = nullptr;
    pipelineInfo.layout = m_rtPostProcessingPipeline.layout;
    pipelineInfo.stage  = stage;

    LOG_DEBUG(VKE, "A");
    CHECK_VK(vkCreateComputePipelines(m_gfx.ctx, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_rtPostProcessingPipeline.pipeline));

    LOG_DEBUG(VKE, "B");

    vkDestroyShaderModule(m_gfx.ctx, computeShader, nullptr);
    LOG_DEBUG(VKE, "Ray tracing post processing pipeline initialized");
}

void VkEngine::DestroyRTPostProcessingPipeline() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying post processing pipeline");

    m_rtPostProcessingPipeline.Destroy(m_gfx.ctx, true);

    LOG_DEBUG(VKE, "Post processing pipeline destroyed");
}

inline uint32_t AlignUp(const uint32_t size, const uint32_t alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}

void VkEngine::InitRayTracingResources() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Initializing RT resources");

    VkImageUsageFlags usages = {};
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usages |= VK_IMAGE_USAGE_STORAGE_BIT;

    const VkFormat format   = m_gfx.targets.draw32f.format;
    const VkExtent3D extent = m_gfx.targets.draw32f.extent;

    m_accumulationImage.format = format;
    m_accumulationImage.extent = extent;

    const VkImageCreateInfo imgInfo = jvk::init::Image(format, usages, extent);

    VmaAllocationCreateInfo vmaInfo{};
    vmaInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(m_gfx.allocator, &imgInfo, &vmaInfo, &m_accumulationImage.image, &m_accumulationImage.allocation, nullptr);

    const VkImageViewCreateInfo viewInfo = jvk::init::ImageView(format, m_accumulationImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(m_gfx.ctx, &viewInfo, nullptr, &m_accumulationImage.view));

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

    LOG_DEBUG(VKE, "RT resources initialized");
}

void VkEngine::DestroyRayTracingResources() {
    TPROFILE_SCOPE();
    LOG_DEBUG(VKE, "Destroying RT resources");

    m_gfx.DestroyBuffer(m_SBT.buffer);
    m_gfx.DestroyImage(m_accumulationImage);

    LOG_DEBUG(VKE, "RT resources destroyed");
}

bool VkEngine::RayTrace(const VkCommandBuffer cmd, const RtRenderSettings &settings, RtRenderState &state, RtRenderTargets &targets) const {
    TPROFILE_SCOPE();

    if (!m_bSceneLoaded) return false;


    const bool bRayTrace            = state.currentSample < settings.targetSamples;
    const bool bApplyPostProcessing = state.bPostProcessSettingsChanged || bRayTrace;

    state.bRenderDone = !bRayTrace;

    if (bApplyPostProcessing) {
        jvk::TransitionImageIfNeeded(cmd, targets.accumulation, targets.accumulationLayout, VK_IMAGE_LAYOUT_GENERAL);
        jvk::TransitionImageIfNeeded(cmd, targets.output, targets.outputLayout, VK_IMAGE_LAYOUT_GENERAL);
        targets.accumulationLayout = VK_IMAGE_LAYOUT_GENERAL;
        targets.outputLayout       = VK_IMAGE_LAYOUT_GENERAL;
    }

    const auto sceneDescriptorSet = m_frameData[m_gfx.GetCurrentFrameIndex()].gpuGlobalUniformDataDescriptorSet;
    const std::vector descriptorSets{sceneDescriptorSet, m_bindlessDescriptorSet};
    if (bRayTrace) {
        const uint32_t nSamples = std::min(settings.targetSamples - state.currentSample, settings.samplesPerFrame);

        RayTracingPushConstants rtpc{};
        rtpc.invView          = state.invView;
        rtpc.invProj          = state.invProj;
        rtpc.currentSample    = state.currentSample;
        rtpc.nSamples         = nSamples;
        rtpc.directClamping   = settings.directClamping;
        rtpc.indirectClamping = settings.indirectClamping;

        state.currentSample += nSamples;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rayTracingPipeline.pipeline);
        vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                m_rayTracingPipeline.layout,
                0,
                (uint32_t) descriptorSets.size(),
                descriptorSets.data(),
                0,
                nullptr);

        vkCmdPushConstants(
                cmd,
                m_rayTracingPipeline.layout,
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                0,
                sizeof(RayTracingPushConstants),
                &rtpc);
        vkCmdTraceRaysKHR(
                cmd,
                &m_SBT.rayGenRegion,
                &m_SBT.missRegion,
                &m_SBT.hitRegion,
                &m_SBT.callableRegion,
                state.width,
                state.height,
                1);

        // The RT shaders output the HDR color to the fp32 draw image.
        // The compute shader will read the color, apply tonemapping/exposure/EOTF/etc
        // and then write it back to the fp32 draw image.
        VkImageMemoryBarrier2 barrier{};
        barrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask     = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        barrier.srcAccessMask    = VK_ACCESS_2_SHADER_WRITE_BIT;
        barrier.dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask    = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        barrier.oldLayout        = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout        = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image            = m_gfx.targets.draw32f.image;
        barrier.subresourceRange = jvk::init::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        VkDependencyInfo dependency{};
        dependency.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2KHR(cmd, &dependency);
    }

    // Post-processing only applied if RT pipeline was invoked or relevant settings changed
    if (bApplyPostProcessing) {
        state.bPostProcessSettingsChanged = false;

        PostProcessingPushConstants pppc;
        pppc.exposure      = EV100ToExposure(settings.postProcessing.EV - settings.postProcessing.EC);
        pppc.tonemappingOp = settings.postProcessing.tonemappingOp;
        pppc.nSamples      = state.currentSample;

        vkCmdPushConstants(cmd,
                           m_rtPostProcessingPipeline.layout,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           0,
                           sizeof(pppc),
                           &pppc);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_rtPostProcessingPipeline.pipeline);

        vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_COMPUTE,
                m_rtPostProcessingPipeline.layout,
                0,
                (uint32_t) descriptorSets.size(),
                descriptorSets.data(),
                0,
                nullptr);

        vkCmdDispatch(cmd, std::ceil(state.width / 8.0), std::ceil(state.height / 8.0), 1);
    }

    return bApplyPostProcessing;
}

}// namespace jtx