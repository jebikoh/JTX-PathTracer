#include "ui_renderer.hpp"
#include "display.hpp"

#include <SDL.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

namespace jtx {

void UIRenderer::init() {
    LOG_INFO(UI, "Initializing UI renderer");

    const VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                              {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                              {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                              {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                              {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes    = poolSizes;

    CHECK_VK(vkCreateDescriptorPool(m_pDisplay->m_ctx, &poolInfo, nullptr, &m_descriptorPool));

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(m_pDisplay->m_pWindow);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance            = m_pDisplay->m_ctx.instance;
    initInfo.PhysicalDevice      = m_pDisplay->m_ctx.physicalDevice;
    initInfo.Device              = m_pDisplay->m_ctx.device;
    initInfo.Queue               = m_pDisplay->m_graphicsQueue.queue;
    initInfo.DescriptorPool      = m_descriptorPool;
    initInfo.MinImageCount       = 3;
    initInfo.ImageCount          = 3;
    initInfo.UseDynamicRendering = true;

    initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_pDisplay->m_swapchain.imageFormat;

    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    // Enable docking
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    setupStyle();

    LOG_INFO(UI, "UI renderer initialized");
}

void UIRenderer::draw(const VkCommandBuffer cmd, const VkImageView targetImageView) const {
    const VkRenderingAttachmentInfo attachment = jvk::init::renderingAttachment(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo renderInfo           = jvk::init::rendering(m_pDisplay->m_swapchain.extent, &attachment, nullptr);

    vkCmdBeginRenderingKHR(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRenderingKHR(cmd);
}

bool UIRenderer::getViewportRectangle(jvk::ViewRectangle &out) const {
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

void UIRenderer::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Draw UI
    setupDockSpace();

    drawConsolePanel();
    drawPropertiesPanel();

    ImGui::Render();
}

bool UIRenderer::processSDLEvents(const SDL_Event &event) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    const ImGuiIO &io = ImGui::GetIO();
    const bool bWantsMouse = io.WantCaptureMouse;
    const bool bWantsKeyboard = io.WantCaptureKeyboard;

    return bWantsMouse || bWantsKeyboard;
}

void UIRenderer::destroy() const {
    LOG_INFO(UI, "Cleaning up UI renderer");
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(m_pDisplay->m_ctx.device, m_descriptorPool, nullptr);
    LOG_INFO(UI, "UI renderer cleaned up");
}

void UIRenderer::setupStyle() {
        ImGuiStyle * style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	// style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	// style->Colors[ImGuiCol_Com] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	// style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	// style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	// style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

    style->WindowMenuButtonPosition = ImGuiDir_None;
}

void UIRenderer::setupDockSpace() {
    // Setup window that will hold the dockspace
    constexpr ImGuiWindowFlags window_flags =
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

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    // Setup dockspace
    const ImGuiID dockSpaceId                   = ImGui::GetID("JTX_DockSpace");
    constexpr ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);
    m_pCentralNode = ImGui::DockBuilderGetCentralNode(dockSpaceId);

    drawMenuBar(dockSpaceId, viewport, dockSpaceFlags);

    ImGui::End();
}

auto UIRenderer::setLayout(const ImGuiID dockSpaceId, const ImGuiViewport *viewport, const ImGuiDockNodeFlags dockSpaceFlags) -> void {
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, dockSpaceFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

    ImGuiID dock_main_id         = dockSpaceId;
    const ImGuiID dock_right_id  = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
    const ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

    ImGui::DockBuilderDockWindow("Properties", dock_right_id);
    ImGui::DockBuilderDockWindow("Console", dock_bottom_id);

    ImGui::DockBuilderFinish(dockSpaceId);
}

void UIRenderer::drawMenuBar(const ImGuiID dockSpaceId, const ImGuiViewport *viewport, const ImGuiDockNodeFlags dockSpaceFlags) {
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
                setLayout(dockSpaceId, viewport, dockSpaceFlags);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void UIRenderer::drawConsolePanel() {
    ImGui::Begin("Console");
    ImGui::Text("Console Output");
    ImGui::End();
}

void UIRenderer::drawPropertiesPanel() {
    ImGui::Begin("Properties");
    ImGui::Text("Properties Panel");
    ImGui::End();
}

}// namespace jtx
