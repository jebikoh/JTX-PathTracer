#pragma once

#include "jvk/jvk.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

class Display;

/**
 * This class is responsible for rendering the UI using ImGui, including initialization
 * cleanup, and event handling.
 */
class UIRenderer {
public:
    UIRenderer(Display *pDisplay) : m_pDisplay(pDisplay) {}

    void init();
    void cleanup();

    /**
     * Custom input handling for UI for custom behavior like mouse focus.
     * Should be called during event polling, before processEvents()
     * @param event SDL event to process
     */
    void handleInput(SDL_Event &event);

    /**
     * ImGui event processing function.
     * @param event SDL event to process
     */
    void processEvents(SDL_Event &event) { ImGui_ImplSDL2_ProcessEvent(&event); }

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

private:
    Display *m_pDisplay = nullptr;
    VkDescriptorPool m_descriptorPool{};
};