#include <editor/editor.hpp>
#include <editor/ui_renderer.hpp>

#include <jvk/init.hpp>

#include <IconsLucide.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <imgui_impl_vulkan.h>
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

void UiDrawContext::EndRectangleBackground() const {
    SetChannelBackground();
    const ImVec2 tMax = ImGui::GetItemRectMax();
    m_drawList->AddRectFilled(
            m_bgState.hMin,
            ImVec2(m_bgState.hMax.x, tMax.y),
            ImGui::GetColorU32(ImVec4(0.129, 0.137, 0.141, 1.0f)),
            m_bgState.rnd,
            ImDrawFlags_RoundCornersAll);
    SetChannelForeground();
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
    SetChannelBackground();
    EndRectangleBackground();
    SetChannelForeground();
}

void UiDrawContext::NewRow(const char *label) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(label).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
    ImGui::SetCursorPosX(posX);
    ImGui::Text("%s", label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

void UIRenderer::Init(const std::function<void()> &importSceneCallback, const std::function<void()> &exportSceneCallback, const std::function<void()> &loadHDRICallback) {
    LOG_INFO(UI, "Initializing UI renderer");

    m_importSceneCallback = importSceneCallback;
    m_exportSceneCallback = exportSceneCallback;
    m_loadHDRICallback    = loadHDRICallback;

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
    ImGui_ImplSDL2_InitForVulkan(m_gfx.window.pWindow);

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
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_gfx.swapchain.format;

    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);

    SetupStyle();
    ImGui_ImplVulkan_CreateFontsTexture();

    // Enable docking
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    LOG_INFO(UI, "UI renderer initialized");
}

void UIRenderer::Draw(RenderContext &ctx, const VkClearValue *clearColor) const {
    jvk::TransitionImageIfNeeded(ctx.cmd, ctx.swapchain.image, ctx.layout.swapchain, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.layout.swapchain = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    const VkRenderingAttachmentInfo attachment = jvk::init::RenderingAttachment(ctx.swapchain.view, clearColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo renderInfo           = jvk::init::Rendering(ctx.swapchain.extent, &attachment, nullptr);

    vkCmdBeginRenderingKHR(ctx.cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ctx.cmd);
    vkCmdEndRenderingKHR(ctx.cmd);
}

bool UIRenderer::GetViewportRectangle(jvk::ViewRectangle &out) const {
    if (!m_pCentralNode) return false;

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

    const auto pos  = m_pCentralNode->Pos;
    const auto size = m_pCentralNode->Size;

    out.x = pos.x * scale.x;
    out.y = pos.y * scale.y;
    out.w = size.x * scale.x;
    out.h = size.y * scale.y;
    return true;
}

void UIRenderer::RegisterViewportBackend(const ViewportBackend id, const char *name, const std::function<void(UiDrawContext &)> &settings) {
    m_viewportBackendSettings[id] = settings;
    m_viewportBackendNames[id]    = name;
}

void UIRenderer::LoadScene(Scene *scene) {
    this->m_pScene = scene;
}

void UIRenderer::NewFrame(SceneUpdate &update) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
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
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Import")) {
                        m_importSceneCallback();
                    }
                    if (ImGui::MenuItem("Export")) {
                        m_exportSceneCallback();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Render Image")) {}
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View")) {
                    if (ImGui::MenuItem("Reset Layout")) {
                        SetLayout(dockSpaceId, viewport, dockSpaceFlags);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }
        }

        ImGui::End();
    }

    static int selectionIndex = 0;
    {
        ImGui::Begin("Hierarchy");

        UiDrawContext ctx;
        ctx.Init();

        if (m_pScene) {
            char buffer[256];
            strncpy(buffer, m_pScene->name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            ImGui::Text("Scene:");
            ImGui::SameLine();
            if (ImGui::InputText("##SceneName", buffer, sizeof(buffer))) {
                m_pScene->name = buffer;
            }
        } else {
            ImGui::Text("Scene: No scene loaded");
        }


        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginListBox("##SceneHierarchy")) {
            if (m_pScene) {
                const auto &meshes = m_pScene->meshes;
                for (int i = 0; i < meshes.size(); i++) {
                    const bool bIsSelected = (selectionIndex == i);
                    if (ImGui::Selectable(meshes[i].name.c_str(), bIsSelected)) selectionIndex = i;
                    if (bIsSelected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        ctx.Destroy();
        ImGui::End();
    }

    // Draw scene settings window
    {
        ImGui::Begin("Object");

        UiDrawContext ctx;
        ctx.Init();

        if (m_pScene != nullptr) {
            char buffer[256];
            strncpy(buffer, m_pScene->meshes[selectionIndex].name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            ImGui::Text("Mesh:");
            ImGui::SameLine();
            if (ImGui::InputText("##MeshName", buffer, sizeof(buffer))) {
                m_pScene->meshes[selectionIndex].name = buffer;
            }

            if (ImGui::CollapsingHeader("Transform")) {
                ctx.StartRectangleBackground();

                if (ctx.StartTable("TransformEditor")) {
                    auto &mesh = m_pScene->meshes[selectionIndex];

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

                ctx.EndRectangleBackground();
            }

            if (ImGui::CollapsingHeader("Material")) {
                ctx.StartRectangleBackground();

                if (ctx.StartTable("MaterialEditor")) {
                    auto &mat                   = m_pScene->materials[m_pScene->meshes[selectionIndex].materialIndex];
                    const char *materialTypes[] = {"Diffuse", "Dielectric", "C. Conductor", "Conductor"};
                    int currentType             = mat.mType;

                    bool bMaterialUpdated = false;

                    ctx.NewRow("BxDF");
                    if (ImGui::Combo("##BxDF", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
                        mat.mType        = static_cast<Material::Type>(currentType);
                        bMaterialUpdated = true;
                    }

                    switch (mat.mType) {
                        case Material::Type::DIFFUSE:
                            ctx.NewRow("Diffuse");
                            bMaterialUpdated |= ImGui::ColorEdit3("##diffuse", &mat.parameters.diffuse.x);

                            ctx.NewRow("Emission");
                            bMaterialUpdated |= ImGui::ColorEdit3("##emission", &mat.parameters.emission.x);

                            ctx.NewRow("Emission Strength");
                            bMaterialUpdated |= ImGui::DragFloat("Strength", &mat.parameters.emissionStrength, 0.5, 0, 10000);
                            break;
                        case Material::Type::DIELECTRIC:
                            ctx.NewRow("IOR");
                            bMaterialUpdated |= ImGui::DragFloat("##ior", &mat.parameters.ior.x, 0.01, 0, 100);

                            ctx.NewRow("Anisotropy");
                            bMaterialUpdated |= ImGui::Checkbox("##anisotropic", &mat.parameters.bAnisotropic);

                            if (mat.parameters.bAnisotropic) {
                                ctx.NewRow("Roughness");
                                bMaterialUpdated |= ImGui::DragFloat2("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                            } else {
                                bMaterialUpdated |= ImGui::DragFloat("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                                mat.parameters.roughness.y = mat.parameters.roughness.x;
                            }

                            ctx.NewRow("Emission");
                            bMaterialUpdated |= ImGui::ColorEdit3("##emission", &mat.parameters.emission.x);

                            ctx.NewRow("Emission Strength");
                            bMaterialUpdated |= ImGui::DragFloat("Strength", &mat.parameters.emissionStrength, 0.5, 0, 10000);
                            break;
                        case Material::Type::COMPLEX_CONDUCTOR:
                            ctx.NewRow("IOR");
                            bMaterialUpdated |= ImGui::DragFloat3("##ior", &mat.parameters.ior.x, 0.01);

                            ctx.NewRow("Absorption");
                            bMaterialUpdated |= ImGui::DragFloat3("##absorption", &mat.parameters.k.x);

                            ctx.NewRow("Anisotropy");
                            bMaterialUpdated |= ImGui::Checkbox("##anisotropic", &mat.parameters.bAnisotropic);

                            if (mat.parameters.bAnisotropic) {
                                ctx.NewRow("Roughness");
                                bMaterialUpdated |= ImGui::DragFloat2("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                            } else {
                                bMaterialUpdated |= ImGui::DragFloat("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                                mat.parameters.roughness.y = mat.parameters.roughness.x;
                            }

                            ctx.NewRow("Emission");
                            bMaterialUpdated |= ImGui::ColorEdit3("##emission", &mat.parameters.emission.x);

                            ctx.NewRow("Emission Strength");
                            bMaterialUpdated |= ImGui::DragFloat("Strength", &mat.parameters.emissionStrength, 0.5, 0, 10000);

                            break;
                        case Material::Type::CONDUCTOR:
                            ctx.NewRow("F0");
                            bMaterialUpdated |= ImGui::ColorEdit3("##f0", &mat.parameters.f0.x);

                            ctx.NewRow("Anisotropy");
                            bMaterialUpdated |= ImGui::Checkbox("##anisotropic", &mat.parameters.bAnisotropic);

                            if (mat.parameters.bAnisotropic) {
                                ctx.NewRow("Roughness");
                                bMaterialUpdated |= ImGui::DragFloat2("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                            } else {
                                bMaterialUpdated |= ImGui::DragFloat("##roughness", &mat.parameters.roughness.x, 0.01, 0, 1);
                                mat.parameters.roughness.y = mat.parameters.roughness.x;
                            }

                            ctx.NewRow("Emission");
                            bMaterialUpdated |= ImGui::ColorEdit3("##emission", &mat.parameters.emission.x);

                            ctx.NewRow("Emission Strength");
                            bMaterialUpdated |= ImGui::DragFloat("Strength", &mat.parameters.emissionStrength, 0.5, 0, 10000);

                            break;
                        default:
                            break;
                    }

                    if (bMaterialUpdated) {
                        update.materialIndex = m_pScene->meshes[selectionIndex].materialIndex;
                        LOG_DEBUG(UI, "Material updated: {}", update.materialIndex);
                    } else {
                        update.materialIndex = -1;
                    }

                    ctx.EndTable();
                }
                ctx.EndRectangleBackground();
            }

            if (ImGui::CollapsingHeader("Environment Map")) {
                ctx.StartRectangleBackground();

                bool bReset = false;
                auto &envmap = m_pScene->envmap;
                bool bHDRI = envmap.type == EnvMap::HDRI;

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
                            m_loadHDRICallback();
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
                            bReset = true;
                            envmap.horizontalOffset = Radians(hOffset);
                        }

                        float vOffset = Degrees(envmap.verticalOffset);
                        ctx.NewRow("Vertical Offset");
                        if (ImGui::DragFloat("##VerticalOffset", &vOffset, 1, 0)) {
                            bReset = true;
                            envmap.verticalOffset = Radians(vOffset);
                        }
                        if (!bHDRI) ImGui::EndDisabled();

                    ctx.EndTable();
                }

                ctx.EndRectangleBackground();
                update.bResetAccumulation = bReset;
            }
        }

        ctx.Destroy();
        ImGui::End();
    }

        // Draw render settings window
    {
        ImGui::Begin("Settings");

        UiDrawContext ctx;
        ctx.Init();

        // Backend table
        if (ctx.StartTable("BackendTable")) {
            const char *renderBackends[]    = {"JTX"};
            static int currentRenderBackend = 0;
            ctx.NewRow("Render Backend");
            ImGui::Combo("##RenderBackend", &currentRenderBackend, renderBackends, IM_ARRAYSIZE(renderBackends));

            ctx.NewRow("Viewport Backend");
            ImGui::Combo("##ViewportBackend", &m_currentViewportBackend, m_viewportBackendNames, IM_ARRAYSIZE(m_viewportBackendNames));

            ctx.EndTable();
        }


#ifdef JTX_UI_DRAW_DEMO_WINDOW
        ImGui::ShowDemoWindow();
#endif

        // Sampling settings
        if (ImGui::CollapsingHeader("Sampling")) {
            static int xPixelSamples = 16;
            static int yPixelSamples = 16;
            static int maxDepth      = 32;

            ctx.StartRectangleBackground();
            if (ctx.StartTable("SamplingTable")) {
                ctx.NewRow("SPP X");
                ImGui::DragInt("##XSamples", &xPixelSamples, 1);

                ctx.NewRow("SPP Y");
                ImGui::DragInt("##YSamples", &yPixelSamples, 1);

                ctx.NewRow("Max Depth");
                ImGui::DragInt("##MaxDepth", &maxDepth, 1);
                ctx.EndTable();
            }
            ctx.EndRectangleBackground();
        }

        if (ImGui::CollapsingHeader("Performance")) {
            static int tileSize       = 32;
            static int numThreads     = 32;
            static int samplesPerPass = 1;

            ctx.StartRectangleBackground();
            if (ctx.StartTable("Performance")) {
                ctx.NewRow("Tile Size");
                ImGui::DragInt("##TileSize", &tileSize, 0);

                ctx.NewRow("Thread Count");
                ImGui::DragInt("##NumThreads", &numThreads, 0);

                ctx.NewRow("Samples Per Pass");
                ImGui::DragInt("##SamplesPerPass", &samplesPerPass, 0);

                ctx.EndTable();
            }
            ctx.EndRectangleBackground();
        }

        if (ImGui::CollapsingHeader("Viewport")) {
            m_viewportBackendSettings[m_currentViewportBackend](ctx);
        }

        ctx.Destroy();
        ImGui::End();
    }

    ImGui::Render();
}

bool UIRenderer::ProcessEvent(const SDL_Event &event) const {
    ImGui_ImplSDL2_ProcessEvent(&event);

    const ImGuiIO &io         = ImGui::GetIO();
    const bool bWantsMouse    = io.WantCaptureMouse;
    const bool bWantsKeyboard = io.WantCaptureKeyboard;

    return bWantsMouse || bWantsKeyboard;
}

void UIRenderer::Destroy() const {
    LOG_INFO(UI, "Cleaning up UI renderer");

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(m_gfx.ctx, m_descriptorPool, nullptr);

    LOG_INFO(UI, "UI renderer cleaned up");
}

void UIRenderer::SetupStyle() const {
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
    style->ScrollbarRounding = 1.0f;
    style->GrabRounding      = 1.0f;
    style->LogSliderDeadzone = 4.0f;
    style->TabRounding       = 2.0f;

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

    // style->Colors[ImGuiCol_Tab]        = DARKER;
    // style->Colors[ImGuiCol_TabHovered] = DARKER_A1;
    // style->Colors[ImGuiCol_TabActive]  = DARKER_A3;

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
    SDL_Vulkan_GetDrawableSize(m_gfx.window.pWindow, &drawableW, &drawableH);

    float dpiScale = static_cast<float>(drawableW) / static_cast<float>(windowW);

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(
            "assets/inter_med.ttf",
            16.0f * dpiScale);
    io.FontGlobalScale = 1.0f / dpiScale;

    const float iconSize                    = 16.0f * dpiScale;
    static constexpr ImWchar icons_ranges[] = {ICON_MIN_LC, ICON_MAX_16_LC, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode        = true;
    icons_config.PixelSnapH       = true;
    icons_config.GlyphMinAdvanceX = iconSize;
    io.Fonts->AddFontFromFileTTF("assets/lucide.ttf", iconSize, &icons_config, icons_ranges);
}

void UIRenderer::SetLayout(const ImGuiID dockSpaceId, const ImGuiViewport *viewport, const ImGuiDockNodeFlags dockSpaceFlags) {
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, dockSpaceFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

    ImGuiID dockMainId           = dockSpaceId;
    ImGuiID dockRightId          = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);
    const ImGuiID dockRightTopId = ImGui::DockBuilderSplitNode(dockRightId, ImGuiDir_Up, 0.25f, nullptr, &dockRightId);

    ImGui::DockBuilderDockWindow("Settings", dockRightId);
    ImGui::DockBuilderDockWindow("Object", dockRightId);
    ImGui::DockBuilderDockWindow("Hierarchy", dockRightTopId);

    ImGui::DockBuilderFinish(dockSpaceId);
}

}// namespace jtx
