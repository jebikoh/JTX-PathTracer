#include "ui_renderer.hpp"
#include "display.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

// #define JTX_UI_DRAW_DEMO_WINDOW

namespace jtx {

void UiDrawContext::Init() {
    m_drawList = ImGui::GetWindowDrawList();
    m_drawList->ChannelsSplit(2);
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
    m_bgState.rnd    = ImGui::GetStyle().FrameRounding;
    m_bgState.hMin   = ImGui::GetItemRectMin();
    m_bgState.hMax   = ImGui::GetItemRectMax();
}

void UiDrawContext::EndRectangleBackground() const {
    const ImVec2 tMax = ImGui::GetItemRectMax();
    m_drawList->AddRectFilled(
            m_bgState.hMin,
            ImVec2(m_bgState.hMax.x, tMax.y),
            ImGui::GetColorU32(ImVec4(0.129, 0.137, 0.141, 1.0f)),
            m_bgState.rnd,
            ImDrawFlags_RoundCornersAll);
}

inline void UiDrawContext::StartTable(const char *id, const float col0, const float col1) {
    StartRectangleBackground();
    m_bTableActive = ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("COL1", ImGuiTableColumnFlags_WidthStretch, col0);
    ImGui::TableSetupColumn("COL2", ImGuiTableColumnFlags_WidthStretch, col1);
}

inline void UiDrawContext::EndTable() {
    if (m_bTableActive) {
        ImGui::EndTable();
        m_bTableActive = false;
        SetChannelBackground();
        EndRectangleBackground();
        SetChannelForeground();
    }
}
inline void UiDrawContext::StartTableNoBackground(const char *id, float col0, float col1) {
    m_bTableActive = ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("COL1", ImGuiTableColumnFlags_WidthStretch, col0);
    ImGui::TableSetupColumn("COL2", ImGuiTableColumnFlags_WidthStretch, col1);
}
inline void UiDrawContext::EndTableNoBackground() {
    if (m_bTableActive) {
        ImGui::EndTable();
        m_bTableActive = false;
        SetChannelBackground();
        EndRectangleBackground();
        SetChannelForeground();
    }
}

inline void UiDrawContext::NewRow(const char *label) const {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(label).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x); \
    ImGui::SetCursorPosX(posX);                                                                                                                                     \
    ImGui::Text("%s", label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

void UIRenderer::Init() {
    LOG_INFO(UI, "Initializing UI renderer");

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

void UIRenderer::NewFrame() {
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
        const ImGuiID dockSpaceId                   = ImGui::GetID("JTX_DockSpace");
        constexpr ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
        ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);
        m_pCentralNode = ImGui::DockBuilderGetCentralNode(dockSpaceId);

        // Draw menubar
        {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New")) {}
                    if (ImGui::MenuItem("Open")) {}
                    if (ImGui::MenuItem("Save")) {}
                    if (ImGui::MenuItem("Exit")) {}
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo")) {}
                    if (ImGui::MenuItem("Redo")) {}
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut")) {}
                    if (ImGui::MenuItem("Copy")) {}
                    if (ImGui::MenuItem("Paste")) {}
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

    // Draw scene settings window
    {
        ImGui::Begin("Scene Settings");
        ImGui::Text("Scene Settings");
        ImGui::End();
    }

    // Draw render settings window
    {
        ImGui::Begin("Render Settings");

        UiDrawContext ctx;
        ctx.Init();
        ctx.SetChannelForeground();

        // Backend table
        {
            ctx.StartTableNoBackground("BackendTable");
            ctx.NewRow("Render Backend");
            const char *renderBackends[]    = {"JTX"};
            static int currentRenderBackend = 0;
            ImGui::Combo("##RenderBackend", &currentRenderBackend, renderBackends, IM_ARRAYSIZE(renderBackends));

            ctx.NewRow("Viewport Backend");
            const char *viewportBackends[] = {"JVK"};
            static int currentVpBackend    = 0;
            ImGui::Combo("##ViewportBackend", &currentVpBackend, viewportBackends, IM_ARRAYSIZE(viewportBackends));
            ctx.EndTableNoBackground();
        }

#ifdef JTX_UI_DRAW_DEMO_WINDOW
        ImGui::ShowDemoWindow();
#endif

        // Sampling settings
        if (ImGui::CollapsingHeader("Sampling")) {
            static int xPixelSamples = 16;
            static int yPixelSamples = 16;
            static int maxDepth      = 32;

            ctx.StartTable("SamplingTable");

            ctx.NewRow("SPP X");
            ImGui::DragInt("##XSamples", &xPixelSamples, 1);

            ctx.NewRow("SPP Y");
            ImGui::DragInt("##YSamples", &yPixelSamples, 1);

            ctx.NewRow("Max Depth");
            ImGui::DragInt("##MaxDepth", &maxDepth, 1);

            ctx.EndTable();
        }

        if (ImGui::CollapsingHeader("Performance")) {
            static int tileSize       = 32;
            static int numThreads     = 32;
            static int samplesPerPass = 1;

            ctx.StartTable("Performance");

            ctx.NewRow("Tile Size");
            ImGui::DragInt("##TileSize", &tileSize, 0);

            ctx.NewRow("Thread Count");
            ImGui::DragInt("##NumThreads", &numThreads, 0);

            ctx.NewRow("Samples Per Pass");
            ImGui::DragInt("##SamplesPerPass", &samplesPerPass, 0);

            ctx.EndTable();
        }

        ctx.Destroy();

        ImGui::CollapsingHeader("Debug");

        // const auto cameraPosition = m_pDisplay->m_rasterizer.m_camera.;
        // ImGui::Text("Current camera pos: (%d, %d, %d)", cameraPosition.x, cameraPosition.y, cameraPosition.z);

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
    style->FramePadding      = ImVec2(4, 3);
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
    style->DockingSeparatorSize = 0.0f;

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

    constexpr auto BACKGROUND = ImVec4(0.086, 0.086, 0.086, 1.0);// #151515

    style->Colors[ImGuiCol_Text]         = BRIGHT;
    style->Colors[ImGuiCol_TextDisabled] = LOW;

    style->Colors[ImGuiCol_WindowBg] = BACKGROUND;
    style->Colors[ImGuiCol_ChildBg]  = BACKGROUND;
    style->Colors[ImGuiCol_PopupBg]  = BACKGROUND;

    style->Colors[ImGuiCol_Border]       = DARK;
    style->Colors[ImGuiCol_BorderShadow] = TRANSPARENT;

    style->Colors[ImGuiCol_FrameBg]        = ImVec4(0, 0, 0, 0.85);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0, 0, 0, 0.95);
    style->Colors[ImGuiCol_FrameBgActive]  = ImVec4(0, 0, 0, 1);

    style->Colors[ImGuiCol_TitleBg]          = DARKER;
    style->Colors[ImGuiCol_TitleBgActive]    = ImVec4(0.159, 0.170, 0.176, 1.0);
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

    style->Colors[ImGuiCol_Tab]        = DARK;
    style->Colors[ImGuiCol_TabHovered] = DARKER;
    style->Colors[ImGuiCol_TabActive]  = BACKGROUND;

    style->Colors[ImGuiCol_TabDimmed]         = DARK;
    style->Colors[ImGuiCol_TabDimmedSelected] = BACKGROUND;

    style->Colors[ImGuiCol_TextSelectedBg] = MEDIUM_A;

    style->WindowMenuButtonPosition = ImGuiDir_None;

    // Font
    int windowW, windowH, drawableW, drawableH;
    SDL_GetWindowSize(m_gfx.window.pWindow, &windowW, &windowH);
    SDL_Vulkan_GetDrawableSize(m_gfx.window.pWindow, &drawableW, &drawableH);

    float dpiScale = static_cast<float>(drawableW) / static_cast<float>(windowW);

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(
            "assets/inter.ttf",
            16.0f * dpiScale);
    io.FontGlobalScale = 1.0f / dpiScale;
}

auto UIRenderer::SetLayout(const ImGuiID dockSpaceId, const ImGuiViewport *viewport, const ImGuiDockNodeFlags dockSpaceFlags) -> void {
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, dockSpaceFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

    ImGuiID dockMainId         = dockSpaceId;
    const ImGuiID dockRightId  = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);
    const ImGuiID dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.25f, nullptr, &dockMainId);

    ImGui::DockBuilderDockWindow("Render Settings", dockRightId);
    ImGui::DockBuilderDockWindow("Scene Settings", dockRightId);

    ImGui::DockBuilderFinish(dockSpaceId);
}

}// namespace jtx
