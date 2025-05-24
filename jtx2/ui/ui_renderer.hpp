#pragma once

#include "jvk/jvk.hpp"
#include "jvk/util.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

struct ImGuiDockNode;

namespace jtx {

class Display;

/**
 * This class is responsible for rendering the UI using ImGui, including initialization
 * destruction, and event handling.
 */
class UIRenderer {
public:
    UIRenderer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void init();
    void destroy() const;

    /**
     * Custom input handling for UI for custom behavior like mouse focus.
     * This should ideally be called before passing the input elsewhere
     * @param event SDL event to process
     * @return true if UI wants to consume the event, false o/w
     */
    static bool processSDLEvents(const SDL_Event &event);

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
     * @param out ViewRectangle containing top left coordinate of viewport window and width/height
     * @return true if position and size were retrieved, false if not (e.g. when central node is is not initialized yet)
     */
    bool getViewportRectangle(jvk::ViewRectangle &out) const;

private:
    Display *m_pDisplay = nullptr;
    VkDescriptorPool m_descriptorPool{};

    // We need to store the central node so we can easily retrieve its dimensions
    ImGuiDockNode *m_pCentralNode{};

    void setupStyle() const;

    // Docking setup
    static void setLayout(ImGuiID dockSpaceId, const ImGuiViewport *viewport, ImGuiDockNodeFlags dockSpaceFlags);
};

}// namespace jtx
