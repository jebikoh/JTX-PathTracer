#pragma once

#include "jvk/jvk.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

struct ImGuiDockNode;

namespace jtx {

class Display;

/**
 * This class is responsible for rendering the UI using ImGui, including initialization
 * cleanup, and event handling.
 */
class UIRenderer {
public:
    UIRenderer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void init();
    void cleanup() const;

    /**
     * Custom input handling for UI for custom behavior like mouse focus.
     * Should be called during event polling, before processEvents()
     * @param event SDL event to process
     */
    static void handleInput(const SDL_Event &event);

    /**
     * ImGui event processing function.
     * @param event SDL event to process
     */
    static void processEvents(const SDL_Event &event) { ImGui_ImplSDL2_ProcessEvent(&event); }

    /**
     * Creates a new draw frame and generates draw data for the UI.
     * Should be called prior to draw(), after input has been handled
     */
    void newFrame();

    /**
     * Submits draw commands for the UI. Expects image to be in VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
     * @param cmd command buffer
     * @param targetImageView image view to draw to
     */
    void draw(VkCommandBuffer cmd, VkImageView targetImageView) const;

    /**
     * Retrieves the position and size of the viewport window
     * @param position position output (top left corner)
     * @param size size output
     */
    void getViewportPosition(vec2 &position, vec2 &size) const;

private:
    Display *m_pDisplay = nullptr;
    VkDescriptorPool m_descriptorPool{};

    // We need to store the central node so we can easily retrieve its dimensions
    ImGuiDockNode *m_centralNode{};

    static void setupStyle();

    // Docking setup
    void setupDockSpace();
    static void setLayout(ImGuiID dockSpaceId, const ImGuiViewport *viewport, ImGuiDockNodeFlags dockSpaceFlags);
    static void drawMenuBar(ImGuiID dockSpaceId, const ImGuiViewport *viewport, ImGuiDockNodeFlags dockSpaceFlags);

    static void drawConsolePanel();
    static void drawPropertiesPanel();
};

}// namespace jtx
