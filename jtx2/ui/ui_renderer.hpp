#pragma once

#include "jvk/jvk.hpp"
#include "jvk/util.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

struct ImGuiDockNode;

namespace jtx {
struct RenderContext;

class Display;
struct GfxContext;

/**
 * This class is responsible for rendering the UI using ImGui, including initialization
 * destruction, and event handling.
 */
class UIRenderer {
public:
    explicit UIRenderer(const GfxContext &gfx)
        : m_gfx(gfx) {}

    void Init();
    void Destroy() const;

    /**
     * UI input handling for custom behavior like mouse focus.
     * This should be called before passing the input elsewhere
     * @param event SDL event to process
     * @return true if UI wants to consume the event, false o/w
     */
    bool ProcessEvent(const SDL_Event &event) const;

    /**
     * Creates a new draw frame and generates draw data for the UI.
     * Should be called prior to draw(), after input has been handled
     */
    void NewFrame();

    /**
     * Submits draw commands for the UI. Expects image to be in VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
     * @param ctx render context
     * @param clearColor optional clear color for the swapchain image
     */
    void Draw(RenderContext &ctx, const VkClearValue *clearColor = nullptr) const;

    /**
     * Retrieves the position and size of the viewport window
     * @param out ViewRectangle containing top left coordinate of viewport window and width/height
     * @return true if position and size were retrieved, false if not (e.g. when central node is not initialized yet)
     */
    bool GetViewportRectangle(jvk::ViewRectangle &out) const;

private:
    const GfxContext &m_gfx;
    VkDescriptorPool m_descriptorPool{};

    // We need to store the central node so we can easily retrieve its dimensions
    ImGuiDockNode *m_pCentralNode{};

    void SetupStyle() const;

    // Docking setup
    static void SetLayout(ImGuiID dockSpaceId, const ImGuiViewport *viewport, ImGuiDockNodeFlags dockSpaceFlags);
};

}// namespace jtx
