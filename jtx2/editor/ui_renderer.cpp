#include <editor/editor.hpp>
#include <editor/ui_renderer.hpp>

#include <engine/cpu/bxdf/microfacet.hpp>
#include <jvk/init.hpp>
#include <util/profiling.hpp>

#include <IconsLucide.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui_internal.h>

// #define JTX_UI_DRAW_DEMO_WINDOW

namespace jtx {

void UiDrawContext::Init() {
    m_drawList = ImGui::GetWindowDrawList();
    m_drawList->ChannelsSplit(2);
    SetChannelForeground();
}

void UiDrawContext::Destroy() const {
    m_drawList->ChannelsMerge();
}

void UiDrawContext::SetChannelForeground() const {
    m_drawList->ChannelsSetCurrent(1);
}

void UiDrawContext::SetChannelBackground() const {
    m_drawList->ChannelsSetCurrent(0);
}

void UiDrawContext::StartRectangleBackground() {
    m_bgState.rnd  = ImGui::GetStyle().FrameRounding;
    m_bgState.hMin = ImGui::GetItemRectMin();
    m_bgState.hMax = ImGui::GetItemRectMax();
}

void UiDrawContext::EndRectangleBackground(const bool bApplyPadding) const {
    const float paddingY = bApplyPadding ? ImGui::GetStyle().ItemSpacing.y : 0.0f;

    SetChannelBackground();
    const ImVec2 tMax = ImGui::GetItemRectMax();
    m_drawList->AddRectFilled(
            m_bgState.hMin,
            ImVec2(m_bgState.hMax.x, tMax.y + paddingY),
            ImGui::GetColorU32(ImVec4(0.129, 0.137, 0.141, 1.0f)),
            m_bgState.rnd,
            ImDrawFlags_RoundCornersAll);
    SetChannelForeground();
    if (bApplyPadding) ImGui::Dummy(ImVec2(0.0f, paddingY));
}

void UiDrawContext::InsertPadding() const {
    const float paddingY = ImGui::GetStyle().ItemSpacing.y;
    ImGui::Dummy(ImVec2(0.0f, paddingY));
}

ImVec2 UiDrawContext::GetAvailWidth() const {
    return ImVec2(ImGui::GetContentRegionAvail().x, 0);
}

bool UiDrawContext::StartTable(const char *id, const float col0, const float col1) const {
    if (ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("COL1", ImGuiTableColumnFlags_WidthStretch, col0);
        ImGui::TableSetupColumn("COL2", ImGuiTableColumnFlags_WidthStretch, col1);
        return true;
    }
    return false;
}

void UiDrawContext::EndTable() const {
    ImGui::EndTable();
    // SetChannelBackground();
    // EndRectangleBackground();
    // SetChannelForeground();
}

void UiDrawContext::NewRow(const char *label) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::AlignTextToFramePadding();
    const float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(label).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
    ImGui::SetCursorPosX(posX);
    ImGui::Text("%s", label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

void UIRenderer::Init(
    const std::function<void()> &onImportSceneCallback,
    const std::function<void()> &onExportSceneCallback,
    const std::function<void()> &onLoadHDRICallback,
    const std::function<void()> &onStartRenderImageCallback,
    const std::function<void()> &onStopRenderImageCallback,
    const std::function<void()> &onSaveRenderImageCallback)
{
    TPROFILE_SCOPE();
    LOG_INFO(UI, "Initializing UI renderer");

    m_onImportSceneCallback = onImportSceneCallback;
    m_onExportSceneCallback = onExportSceneCallback;
    m_onLoadHDRICallback    = onLoadHDRICallback;
    m_onStartRenderImageCallback = onStartRenderImageCallback;
    m_onStopRenderImageCallback = onStopRenderImageCallback;
    m_onSaveRenderImageCallback = onSaveRenderImageCallback;

    constexpr VkDescriptorPoolSize poolSizes[] =
            {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
            };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes    = poolSizes;

    CHECK_VK(vkCreateDescriptorPool(m_gfx.ctx, &poolInfo, nullptr, &m_descriptorPool));

    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(m_gfx.window.pWindow);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance            = m_gfx.ctx;
    initInfo.PhysicalDevice      = m_gfx.ctx;
    initInfo.Device              = m_gfx.ctx;
    initInfo.Queue               = m_gfx.graphicsQueue.queue;
    initInfo.DescriptorPool      = m_descriptorPool;
    initInfo.MinImageCount       = 3;
    initInfo.ImageCount          = 3;
    initInfo.UseDynamicRendering = true;

    initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_gfx.window.swapchain.format;

    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);

    SetupStyle();

    // Enable docking
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    LOG_INFO(UI, "UI renderer initialized");
}

void UIRenderer::Draw(RenderContext &ctx, const VkClearValue *clearColor) const {
    TPROFILE_SCOPE();
    jvk::TransitionImageIfNeeded(ctx.cmd, ctx.swapchain.image, ctx.layout.swapchain, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.layout.swapchain = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    const VkRenderingAttachmentInfo attachment = jvk::init::RenderingAttachment(ctx.swapchain.view, clearColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo renderInfo           = jvk::init::Rendering(ctx.swapchain.extent, &attachment, nullptr);

    vkCmdBeginRenderingKHR(ctx.cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ctx.cmd);
    vkCmdEndRenderingKHR(ctx.cmd);
}

bool UIRenderer::GetViewportRectangle(jvk::ViewRectangle &out) const {
    TPROFILE_SCOPE();
    if (!m_pCentralNode) return false;

    const ImGuiViewport *mainViewport = ImGui::GetMainViewport();

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

    ImVec2 minPos = m_pCentralNode->Pos;
    minPos.x -= mainViewport->Pos.x;
    minPos.y -= mainViewport->Pos.y;

    const auto size = m_pCentralNode->Size;

    out.x = minPos.x * scale.x;
    out.y = minPos.y * scale.y;
    out.w = size.x * scale.x;
    out.h = size.y * scale.y;

    return true;
}

void UIRenderer::RegisterViewportBackend(const ViewportBackend id, const char *name, const std::function<void(UiDrawContext &)> &settings) {
    m_viewportBackendSettings[id] = settings;
    m_viewportBackendNames[id]    = name;
}

void UIRenderer::RegisterRenderBackend(const RenderBackend id, const char *name, const std::function<bool(UiDrawContext &)> &settings) {
    m_renderBackendNames[id]    = name;
    m_renderBackendPanels[id] = settings;
}

void UIRenderer::LoadScene(Scene *scene) {
    this->m_pScene = scene;
    m_materials.clear();
    for (const auto &mat: scene->materials) {
        m_materials.append(mat.name);
        m_materials.push_back('\0');
    }
}

void UIRenderer::NewFrame(SceneUpdate &update) {
    TPROFILE_SCOPE();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Draw dockspace
    {
        constexpr ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        // Setup dockspace
        const ImGuiID dockSpaceId = ImGui::GetID("JTX_DockSpace");
        bool bDockSpaceNotCreated = ImGui::DockBuilderGetNode(dockSpaceId) == nullptr;

        constexpr ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
        ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);
        m_pCentralNode = ImGui::DockBuilderGetCentralNode(dockSpaceId);

        if (bDockSpaceNotCreated) {
            SetLayout(dockSpaceId, viewport, dockSpaceFlags);
        }

        // Draw menubar
        {
            if (ImGui::BeginMenuBar()) {
                ImGui::BeginDisabled(m_bRenderWindowOpen);
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Import")) {
                        m_onImportSceneCallback();
                    }
                    if (ImGui::MenuItem("Export")) {
                        m_onExportSceneCallback();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Render")) {
                    if (m_pScene == nullptr) ImGui::BeginDisabled();
                    if (ImGui::MenuItem("Render Image")) {
                        m_onStartRenderImageCallback();
                        m_bRenderWindowOpen = true;
                    }
                    if (m_pScene == nullptr) ImGui::EndDisabled();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View")) {
                    if (ImGui::MenuItem("Reset Layout")) {
                        SetLayout(dockSpaceId, viewport, dockSpaceFlags);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndDisabled();
                ImGui::EndMenuBar();
            }
        }

        ImGui::End();
    }

    if (m_bRenderWindowOpen) {
        TPROFILE_SCOPE_N("Render Window");
        ImGuiWindowClass windowClass;
        windowClass.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther | ImGuiDockNodeFlags_NoDockingSplitOther;
        windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin("JTX Render", &m_bRenderWindowOpen, ImGuiWindowFlags_NoCollapse)) {
            ImGui::PushStyleVar(ImGuiStyleVar_DockingSeparatorSize, 0.0f);

            ImGuiID dockspaceId = ImGui::GetID("RenderPopupDockspace");
            ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

            static bool bFirstTime = true;
            if (bFirstTime) {
                bFirstTime = false;

                ImGui::DockBuilderRemoveNode(dockspaceId);
                ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

                ImGuiID dockMainId = dockspaceId;
                ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);
                ImGuiID dockLeftId = dockMainId;

                ImGui::DockBuilderDockWindow("Render Image", dockLeftId);
                ImGui::DockBuilderDockWindow("Render Settings", dockRightId);
                ImGui::DockBuilderFinish(dockspaceId);
            }

            ImGui::SetNextWindowClass(&windowClass);
            ImGui::Begin("Render Settings", nullptr, ImGuiWindowFlags_NoTitleBar);

            UiDrawContext ctx;
            ctx.Init();

            // TODO: this is scuffed, change this later
            bool bRenderDone = m_renderBackendPanels[m_currentViewportBackend](ctx);
            if (!bRenderDone) ImGui::BeginDisabled();
            if (ImGui::Button("Save", ctx.GetAvailWidth())) {
                m_onSaveRenderImageCallback();
            }
            if (!bRenderDone) ImGui::EndDisabled();

            ImGui::End();

            ImGui::SetNextWindowClass(&windowClass);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Render Image", nullptr, ImGuiWindowFlags_NoTitleBar);
            ImGui::PopStyleVar();

            ImVec2 availableRegion = ImGui::GetContentRegionAvail();
            float regionAspectRatio = availableRegion.x / availableRegion.y;

            float imageWidth = static_cast<float>(m_rs.width);
            float imageHeight = static_cast<float>(m_rs.height);
            float imageAspectRatio = imageWidth / imageHeight;

            ImVec2 imageSize;

            if (imageAspectRatio > regionAspectRatio) {
                imageSize.x = availableRegion.x;
                imageSize.y = availableRegion.x / imageAspectRatio;
            } else {
                imageSize.y = availableRegion.y;
                imageSize.x = availableRegion.y * imageAspectRatio;
            }

            float xPos = (availableRegion.x - imageSize.x) * 0.5f;
            float yPos = (availableRegion.y - imageSize.y) * 0.5f;

            ImGui::SetCursorPos(ImVec2(xPos, yPos));
            ImGui::Image(m_renderImage, imageSize, {0, 1}, {1, 0});

            ImGui::End();

            ctx.Destroy();
            ImGui::PopStyleVar();

        }
        ImGui::End();

        if (!m_bRenderWindowOpen) {
            m_onStopRenderImageCallback();
        }
    }

    ImGui::BeginDisabled(m_bRenderWindowOpen);

    // Hierarchy
    static int objSelectionIndex = 0;
    {
        ImGui::Begin("Hierarchy");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginListBox("##SceneHierarchy")) {
            if (m_pScene) {
                const auto &meshes = m_pScene->meshes;
                for (int i = 0; i < meshes.size(); i++) {
                    const bool bIsSelected = (objSelectionIndex == i);
                    if (ImGui::Selectable(meshes[i].name.c_str(), bIsSelected)) objSelectionIndex = i;
                    if (bIsSelected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        ImGui::End();
    }

    // Settings
    {
        if (!m_bStartupFocusSet) {
            ImGui::SetNextWindowFocus();
            m_bStartupFocusSet = true;
        }

        ImGui::Begin("Settings");

        UiDrawContext ctx;
        ctx.Init();

        // Backend table
        if (ctx.StartTable("BackendTable")) {
            ctx.NewRow("Render Backend");
            ImGui::Combo("##RenderBackend", &m_currentRenderBackend, m_renderBackendNames, IM_ARRAYSIZE(m_renderBackendNames));

            ctx.NewRow("Viewport Backend");
            ImGui::Combo("##ViewportBackend", &m_currentViewportBackend, m_viewportBackendNames, IM_ARRAYSIZE(m_viewportBackendNames));

            ctx.EndTable();
        }


#ifdef JTX_UI_DRAW_DEMO_WINDOW
        ImGui::ShowDemoWindow();
#endif

        if (ImGui::CollapsingHeader("Render")) {
            ctx.StartRectangleBackground();
            if (ctx.StartTable("RenderSettingsTable")) {
                ctx.NewRow("Render Width");
                ImGui::DragInt("##RenderWidth", reinterpret_cast<int *>(&m_rs.width), 1, 1, 4096);
                ctx.NewRow("Render Height");
                ImGui::DragInt("##RenderHeight", reinterpret_cast<int *>(&m_rs.height), 1, 1, 4096);
                ctx.NewRow("Samples Per Pixel");
                ImGui::DragInt("##RSamplesPerPixel", reinterpret_cast<int *>(&m_rs.spp), 1, 1, 100000);
                ctx.NewRow("Samples Per Pass");
                ImGui::DragInt("##RSamplesPerPass", reinterpret_cast<int *>(&m_rs.samplesPerPass), 1, 1, 1080);
                ctx.NewRow("Max Depth");
                ImGui::DragInt("##MaxDepth", reinterpret_cast<int *>(&m_rs.maxDepth), 1, 1, 1080);
                ctx.NewRow("RNG Seed");
                ImGui::InputInt("##RNGSeed", reinterpret_cast<int *>(&m_rs.seed));

                ctx.EndTable();

                if (ImGui::TreeNode("Post Processing")) {
                    if (ctx.StartTable("Post Processing")) {
                        ctx.NewRow("Exposure");
                        ImGui::InputFloat("##Exposure", &m_rs.EC, 1);

                        const char *tmo[]      = {"None", "Reinhard", "ACES", "AgX", "Hable Filmic"};
                        static int selectedTmo = m_rs.tonemapOp;
                        ctx.NewRow("Tonemapping");
                        if (ImGui::Combo("##TMO", &selectedTmo, tmo, IM_ARRAYSIZE(tmo))) {
                            m_rs.tonemapOp    = static_cast<kTonemapOp>(selectedTmo);
                        }
                        ctx.EndTable();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Clamping")) {
                    if (ctx.StartTable("Clamping")) {
                        ctx.NewRow("Direct Lighting");
                        ImGui::DragFloat("##DirectLighting", &m_rs.directClamping, 0.1f, 0.0f, 100.0f);
                        ctx.NewRow("Indirect Lighting");
                        ImGui::DragFloat("##IndirectLighting", &m_rs.indirectClamping, 0.1f, 0.0f, 100.0f);
                        ctx.EndTable();
                    }

                    ImGui::TreePop();
                }
            }
            ctx.EndRectangleBackground(true);
        }

        if (ImGui::CollapsingHeader("Viewport")) {
            m_viewportBackendSettings[m_currentViewportBackend](ctx);
        }

        ctx.Destroy();
        ImGui::End();
    }

    // Scene
    {
        ImGui::Begin("Scene");

        UiDrawContext ctx;
        ctx.Init();

        char buffer[256];
        if (m_pScene) {
            strncpy(buffer, m_pScene->name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        } else {
            strncpy(buffer, "No scene loaded", sizeof(buffer));
        }

        if (ctx.StartTable("SceneNameTable", 1, 3)) {
            ctx.NewRow("Name");
            ImGui::BeginDisabled(m_pScene == nullptr);
            if (ImGui::InputText("##SceneName", buffer, sizeof(buffer))) {
                m_pScene->name = buffer;
            }
            ImGui::EndDisabled();
            ctx.EndTable();
        }
        ctx.InsertPadding();

        if (m_pScene != nullptr) {
            if (ImGui::CollapsingHeader("Environment Map")) {
                ctx.StartRectangleBackground();

                bool bReset  = false;
                auto &envmap = m_pScene->envmap;
                bool bHDRI   = envmap.type == EnvMap::HDRI;

                if (ctx.StartTable("SkyEditor")) {

                    if (bHDRI) ImGui::BeginDisabled();
                    ctx.NewRow("Uniform Color");
                    bReset |= ImGui::ColorEdit3("##Sky", &envmap.uniform.x);

                    ctx.NewRow("Intensity");
                    bReset |= ImGui::DragFloat("##Intensity", &envmap.intensity, 0.1);
                    if (bHDRI) ImGui::EndDisabled();

                    ctx.NewRow("HDRI");
                    bReset |= ImGui::Checkbox("##HDRI", &bHDRI);
                    envmap.type = bHDRI ? EnvMap::HDRI : EnvMap::UNIFORM;

                    if (!bHDRI) ImGui::BeginDisabled();

                    ctx.NewRow("");
                    if (ImGui::Button("Select Image", ctx.GetAvailWidth())) {
                        bReset = true;
                        m_onLoadHDRICallback();
                    }

                    ctx.NewRow("Map");
                    if (envmap.image.path.empty()) {
                        ImGui::Text("No map loaded");
                    } else {
                        ImGui::Text(envmap.image.path.c_str());
                    }

                    float hOffset = Degrees(envmap.horizontalOffset);
                    ctx.NewRow("Horizontal Offset");
                    if (ImGui::DragFloat("##HorizontalOffset", &hOffset, 1, 0)) {
                        bReset                  = true;
                        envmap.horizontalOffset = Radians(hOffset);
                    }

                    float vOffset = Degrees(envmap.verticalOffset);
                    ctx.NewRow("Vertical Offset");
                    if (ImGui::DragFloat("##VerticalOffset", &vOffset, 1, 0)) {
                        bReset                = true;
                        envmap.verticalOffset = Radians(vOffset);
                    }
                    if (!bHDRI) ImGui::EndDisabled();

                    ctx.EndTable();
                }

                ctx.EndRectangleBackground(true);
                update.bResetAccumulation = bReset;
            }

            if (ImGui::CollapsingHeader("Camera")) {
                ctx.StartRectangleBackground();
                auto &camera = m_pScene->camera;

                if (ImGui::TreeNode("Orientation")) {
                    if (ctx.StartTable("CameraLocation")) {
                        ctx.NewRow("Position");
                        ImGui::InputFloat3("##Position", &camera.position.x);
                        ctx.NewRow("Target");
                        ImGui::InputFloat3("##Target", &camera.target.x);
                        ctx.NewRow("Up");
                        ImGui::InputFloat3("##Up", &camera.up.x);
                        ctx.EndTable();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Lens & Sensor")) {
                    if (ImGui::TreeNode("FOV")) {
                        if (ctx.StartTable("RenderLensFOVTable")) {
                            ctx.NewRow("Focal Length");
                            ImGui::DragFloat("##FocalLength", &camera.settings.focalLength, 0.01f, 0.01f, 100.0f);
                            ctx.NewRow("Sensor Width");
                            ImGui::DragFloat("##SensorWidth", &camera.settings.sensorWidth, 0.01f, 0.01f, 100.0f);
                            ctx.EndTable();
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("DoF")) {
                        if (ctx.StartTable("RenderLensDoFTable")) {
                            ctx.NewRow("Enable DoF");
                            ImGui::Checkbox("##EnableDoF", &camera.settings.bEnableDof);
                            ctx.NewRow("Focal Distance");
                            ImGui::DragFloat("##FocalDistance", &camera.settings.focalDistance, 1.0f, 0.1f, 100000.0f);
                            ctx.NewRow("f-Stop");
                            ImGui::DragFloat("##fStop", &camera.settings.fStop, 0.1f, 0.0f, 10.0f);
                            ctx.EndTable();
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Exposure")) {
                        if (ctx.StartTable("RenderLensExposureTable")) {
                            ctx.NewRow("Shutter Speed");
                            ImGui::DragFloat("##ShutterSpeed", &camera.settings.shutterSpeed, 0.01f, 0.001f, 1000.0f);
                            ctx.NewRow("ISO");
                            ImGui::DragFloat("ISO", &camera.settings.ISO, 1.0f, 0.01f, 1000.0f);
                            ctx.EndTable();
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

                ctx.EndRectangleBackground(true);
            }
        } else {
            ImGui::BeginDisabled();

            ImGui::CollapsingHeader("Environment Map");
            ImGui::CollapsingHeader("Render Camera");

            ImGui::EndDisabled();
        }

        ctx.Destroy();
        ImGui::End();
    }

    // Object
    {
        ImGui::Begin("Object");

        UiDrawContext ctx;
        ctx.Init();

        char buffer[256];
        if (m_pScene) {
            strncpy(buffer, m_pScene->meshes[objSelectionIndex].name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        } else {
            strncpy(buffer, "No mesh selected", sizeof(buffer));
        }

        if (ctx.StartTable("MeshNameTable", 1, 3)) {
            ctx.NewRow("Name");
            ImGui::BeginDisabled(m_pScene == nullptr);
            if (ImGui::InputText("##MeshName", buffer, sizeof(buffer))) {
                m_pScene->meshes[objSelectionIndex].name = buffer;
            }
            ImGui::EndDisabled();
            ctx.EndTable();
        }
        ctx.InsertPadding();

        if (m_pScene != nullptr) {
            if (ImGui::CollapsingHeader("Transform")) {
                ctx.StartRectangleBackground();

                if (ctx.StartTable("TransformEditor")) {
                    auto &mesh = m_pScene->meshes[objSelectionIndex];

                    vec3 position;
                    vec3 rotation;
                    vec3 scale;

                    ctx.NewRow("Position");
                    ImGui::DragFloat3("##Position", &position.x);

                    ctx.NewRow("Rotation");
                    ImGui::DragFloat3("##Rotation", &rotation.x);

                    ctx.NewRow("Scale");
                    ImGui::DragFloat3("##Scale", &scale.x);

                    ctx.EndTable();
                }

                ctx.EndRectangleBackground(true);
            }

            if (ImGui::CollapsingHeader("Material")) {
                ctx.StartRectangleBackground();
                int matIndex = m_pScene->meshes[objSelectionIndex].materialIndex;
                auto &mat           = m_pScene->materials[matIndex];

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::Combo("##Material", &matIndex, m_materials.c_str())) {
                    m_pScene->meshes[objSelectionIndex].materialIndex = matIndex;
                    update.objectIndex = objSelectionIndex;
                }

                ImGui::Separator();

                if (ctx.StartTable("MaterialEditor")) {
                    const char *materialTypes[] = {"Diffuse", "Dielectric", "C. Conductor", "Conductor", "Thin Dielectric", "Glossy Diffuse"};
                    int currentType             = mat.mType;

                    bool bMaterialUpdated = false;

                    strncpy(buffer, mat.name.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    ctx.NewRow("Name");
                    if (ImGui::InputText("##MaterialName", buffer, sizeof(buffer))) {
                        mat.name = buffer;
                    }

                    ctx.NewRow("BxDF");
                    if (ImGui::Combo("##BxDF", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
                        mat.mType        = static_cast<Material::Type>(currentType);
                        bMaterialUpdated = true;
                    }

                    bool bMaterialHasRoughness = false;

                    switch (mat.mType) {
                        case Material::Type::DIFFUSE:
                            ctx.NewRow("Diffuse");
                            bMaterialUpdated |= ImGui::ColorEdit3("##diffuse", &mat.parameters.diffuse.x);
                            break;
                        case Material::Type::DIELECTRIC:
                            ctx.NewRow("IOR");
                            bMaterialUpdated |= ImGui::DragFloat("##ior", &mat.parameters.ior.x, 0.01, 0, 100);
                            bMaterialHasRoughness = true;
                            break;
                        case Material::Type::COMPLEX_CONDUCTOR:
                            ctx.NewRow("IOR");
                            bMaterialUpdated |= ImGui::DragFloat3("##ior", &mat.parameters.ior.x, 0.01);

                            ctx.NewRow("Absorption");
                            bMaterialUpdated |= ImGui::DragFloat3("##absorption", &mat.parameters.k.x);

                            bMaterialHasRoughness = true;
                            break;
                        case Material::Type::CONDUCTOR:
                            ctx.NewRow("F0");
                            bMaterialUpdated |= ImGui::ColorEdit3("##f0", &mat.parameters.f0.x);

                            bMaterialHasRoughness = true;
                            break;
                        case Material::Type::THIN_DIELECTRIC:
                            ctx.NewRow("IOR");
                            bMaterialUpdated |= ImGui::DragFloat("##ior", &mat.parameters.ior.x, 0.01, 0, 100);
                            bMaterialHasRoughness = false;
                            break;
                        case Material::Type::GLOSSY_DIFFUSE:
                            ctx.NewRow("Diffuse");
                            bMaterialUpdated |= ImGui::ColorEdit3("##diffuse", &mat.parameters.diffuse.x);

                            ctx.NewRow("Specular Tint");
                            bMaterialUpdated |= ImGui::SliderFloat("##SpecularTint", &mat.parameters.specularTint, 0.0f, 1.0f);

                            bMaterialHasRoughness = true;
                            break;
                        default:
                            break;
                    }

                    if (bMaterialHasRoughness) {
                        ctx.NewRow("Roughness");
                        bMaterialUpdated |= ImGui::SliderFloat("##Roughness", &mat.parameters.roughness, 0.0f, 1.0f);

                        ctx.NewRow("Anisotropy");
                        bMaterialUpdated |= ImGui::SliderFloat("##Anisotropy", &mat.parameters.anisotropy, -1.0f, 1.0f);
                    }

                    ctx.NewRow("Emission");
                    bMaterialUpdated |= ImGui::ColorEdit3("##emission", &mat.parameters.emission.x);

                    ctx.NewRow("Emission Strength");
                    bMaterialUpdated |= ImGui::DragFloat("Strength", &mat.parameters.emissionStrength, 0.5, 0, 10000);

                    if (bMaterialUpdated) {
                        update.materialIndex = m_pScene->meshes[objSelectionIndex].materialIndex;
                    } else {
                        update.materialIndex = -1;
                    }

                    ctx.EndTable();
                }
                ctx.EndRectangleBackground(true);
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::CollapsingHeader("Transform");
            ImGui::CollapsingHeader("Material");
            ImGui::EndDisabled();
        }

        ctx.Destroy();
        ImGui::End();
    }

    ImGui::EndDisabled();

    ImGui::EndFrame();

    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

bool UIRenderer::ProcessEvent(const SDL_Event &event) const {
    TPROFILE_SCOPE();
    ImGui_ImplSDL3_ProcessEvent(&event);

    const ImGuiIO &io         = ImGui::GetIO();
    const bool bWantsMouse    = io.WantCaptureMouse;
    const bool bWantsKeyboard = io.WantCaptureKeyboard;

    return bWantsMouse || bWantsKeyboard;
}

void UIRenderer::Destroy() const {
    TPROFILE_SCOPE();
    LOG_INFO(UI, "Cleaning up UI renderer");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_gfx.ctx, m_descriptorPool, nullptr);

    LOG_INFO(UI, "UI renderer cleaned up");
}

void UIRenderer::SetupStyle() const {
    TPROFILE_SCOPE();
    ImGuiStyle *style = &ImGui::GetStyle();

    style->WindowPadding     = ImVec2(12, 8);
    style->FramePadding      = ImVec2(8, 3);
    style->CellPadding       = ImVec2(4, 2);
    style->ItemSpacing       = ImVec2(7, 3);
    style->ItemInnerSpacing  = ImVec2(4, 4);
    style->TouchExtraPadding = ImVec2(0, 0);
    style->IndentSpacing     = 21.0f;
    style->ScrollbarSize     = 14.0f;
    style->GrabMinSize       = 20.0f;

    style->WindowBorderSize = 1.0f;
    style->ChildBorderSize  = 1.0f;
    style->PopupBorderSize  = 1.0f;
    style->FrameBorderSize  = 0.0f;
    style->TabBorderSize    = 0.0f;

    style->WindowRounding    = 1.0f;
    style->ChildRounding     = 0.0f;
    style->FrameRounding     = 1.0f;
    style->PopupRounding     = 0.0f;
    style->ScrollbarSize     = 1.0f;
    style->ScrollbarRounding = 1.0f;
    style->GrabRounding      = 1.0f;
    style->LogSliderDeadzone = 4.0f;
    style->TabRounding       = 2.0f;

    style->CellPadding    = ImVec2(0, 1);
    style->TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;

    style->TabBarOverlineSize   = 0.0f;
    style->DockingSeparatorSize = 2.0f;

    // Color palette taken from: https://github.com/ocornut/imgui/issues/5886#issuecomment-1553902053
    constexpr auto TRANSPARENT = ImVec4(0.0, 0.0, 0.0, 0.0);// #00000000
    constexpr auto WHITE       = ImVec4(1.0, 1.0, 1.0, 1.0);// #FFFFFFFF
    constexpr auto BLACK       = ImVec4(0.0, 0.0, 0.0, 1.0);// #000000FF

    constexpr auto BRIGHT = ImVec4(1.0, 0.937, 0.831, 1.0);// #FFEED3

    constexpr auto MEDIUM   = ImVec4(0.8, 0.718, 0.565, 1.0);// #CCB790
    constexpr auto MEDIUM_A = ImVec4(0.8, 0.718, 0.565, 0.6);

    constexpr auto LOW    = ImVec4(0.48, 0.449, 0.392, 1.0);// #7A7263
    constexpr auto LOW_A1 = ImVec4(0.48, 0.449, 0.392, 0.8);

    constexpr auto DARK    = ImVec4(0.298, 0.329, 0.349, 1.0);// #4B5358
    constexpr auto DARK_A1 = ImVec4(0.298, 0.329, 0.349, 0.8);
    constexpr auto DARK_A2 = ImVec4(0.298, 0.329, 0.349, 0.7);

    constexpr auto DARKER    = ImVec4(0.178, 0.191, 0.199, 1.0);// #2D3032
    constexpr auto DARKER_A1 = ImVec4(0.178, 0.191, 0.199, 0.8);
    constexpr auto DARKER_A2 = ImVec4(0.178, 0.191, 0.199, 0.7);
    constexpr auto DARKER_A3 = ImVec4(0.178, 0.191, 0.199, 0.4);

    constexpr auto BACKGROUND         = ImVec4(0.086, 0.086, 0.086, 1.0);// #151515
    constexpr auto BACKGROUND_LIGHTER = ImVec4(0.2, 0.2, 0.2, 1.0);      // #212222

    style->Colors[ImGuiCol_Text]         = BRIGHT;
    style->Colors[ImGuiCol_TextDisabled] = LOW;

    style->Colors[ImGuiCol_WindowBg] = BACKGROUND;
    style->Colors[ImGuiCol_ChildBg]  = BACKGROUND;
    style->Colors[ImGuiCol_PopupBg]  = BACKGROUND;

    style->Colors[ImGuiCol_Border]       = BLACK;
    style->Colors[ImGuiCol_BorderShadow] = TRANSPARENT;

    style->Colors[ImGuiCol_FrameBg]        = ImVec4(0, 0, 0, 0.85);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0, 0, 0, 0.95);
    style->Colors[ImGuiCol_FrameBgActive]  = ImVec4(0, 0, 0, 1);

    style->Colors[ImGuiCol_TitleBg]          = BACKGROUND;
    style->Colors[ImGuiCol_TitleBgActive]    = BACKGROUND;
    style->Colors[ImGuiCol_TitleBgCollapsed] = BACKGROUND;

    style->Colors[ImGuiCol_MenuBarBg] = BACKGROUND;

    style->Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0.8);
    style->Colors[ImGuiCol_ScrollbarGrab]        = DARKER;
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = DARKER_A1;
    style->Colors[ImGuiCol_ScrollbarGrabActive]  = DARKER_A2;

    style->Colors[ImGuiCol_CheckMark] = BRIGHT;

    style->Colors[ImGuiCol_SliderGrab]       = LOW;
    style->Colors[ImGuiCol_SliderGrabActive] = LOW_A1;

    style->Colors[ImGuiCol_Button]        = DARK;
    style->Colors[ImGuiCol_ButtonHovered] = DARK_A1;
    style->Colors[ImGuiCol_ButtonActive]  = DARK_A2;

    style->Colors[ImGuiCol_Header]        = DARKER;
    style->Colors[ImGuiCol_HeaderHovered] = DARKER_A1;
    style->Colors[ImGuiCol_HeaderActive]  = DARKER_A2;

    style->Colors[ImGuiCol_Separator]        = DARKER;
    style->Colors[ImGuiCol_SeparatorHovered] = DARKER;
    style->Colors[ImGuiCol_SeparatorActive]  = DARKER;

    style->Colors[ImGuiCol_ResizeGrip]        = DARKER;
    style->Colors[ImGuiCol_ResizeGripHovered] = DARKER_A1;
    style->Colors[ImGuiCol_ResizeGripActive]  = DARKER_A2;


    style->Colors[ImGuiCol_Tab]        = BACKGROUND;
    style->Colors[ImGuiCol_TabHovered] = BACKGROUND_LIGHTER;
    style->Colors[ImGuiCol_TabActive]  = DARKER;

    style->Colors[ImGuiCol_TabDimmed]         = BACKGROUND;
    style->Colors[ImGuiCol_TabDimmedSelected] = DARKER;

    style->Colors[ImGuiCol_TextSelectedBg] = MEDIUM_A;

    style->WindowMenuButtonPosition = ImGuiDir_None;

    // Font
    int windowW, windowH, drawableW, drawableH;
    SDL_GetWindowSize(m_gfx.window.pWindow, &windowW, &windowH);
    SDL_GetWindowSizeInPixels(m_gfx.window.pWindow, &drawableW, &drawableH);

    float dpiScale = static_cast<float>(drawableW) / static_cast<float>(windowW);

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(
            "../assets/inter_med.ttf",
            16.0f * dpiScale);
    io.FontGlobalScale = 1.0f / dpiScale;

    const float iconSize                    = 16.0f * dpiScale;
    static constexpr ImWchar icons_ranges[] = {ICON_MIN_LC, ICON_MAX_16_LC, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode        = true;
    icons_config.PixelSnapH       = true;
    icons_config.GlyphMinAdvanceX = iconSize;
    io.Fonts->AddFontFromFileTTF("../assets/lucide.ttf", iconSize, &icons_config, icons_ranges);
}

void UIRenderer::SetLayout(const ImGuiID dockSpaceId, const ImGuiViewport *viewport, const ImGuiDockNodeFlags dockSpaceFlags) {
    TPROFILE_SCOPE();
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, dockSpaceFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

    ImGuiID dockMainId           = dockSpaceId;
    ImGuiID dockRightId          = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);
    const ImGuiID dockRightTopId = ImGui::DockBuilderSplitNode(dockRightId, ImGuiDir_Up, 0.25f, nullptr, &dockRightId);

    ImGui::DockBuilderDockWindow("Settings", dockRightId);
    ImGui::DockBuilderDockWindow("Object", dockRightId);
    ImGui::DockBuilderDockWindow("Scene", dockRightId);
    ImGui::DockBuilderDockWindow("Hierarchy", dockRightTopId);

    ImGui::DockBuilderFinish(dockSpaceId);

    ImGuiDockNode *rightNode = ImGui::DockBuilderGetNode(dockRightId);
    if (rightNode) {
        rightNode->SelectedTabId = ImHashStr("Settings");
    }
}

}// namespace jtx
