#pragma once

#include <jvk/jvk.hpp>
#include <jvk/util.hpp>

#include <engine/backends.hpp>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <scene/scene.hpp>

struct ImGuiDockNode;

namespace jtx {
struct RenderContext;

class Editor;
struct GfxContext;

struct UiDrawContext {
    void Init();
    void Destroy() const;

    bool StartTable(const char *id, float col0 = 1.0f, float col1 = 1.0f) const;
    void EndTable() const;

    static void NewRow(const char *label);

    void StartRectangleBackground();
    void EndRectangleBackground(bool bApplyPadding = false) const;

    ImVec2 GetAvailWidth() const;

private:
    ImDrawList *m_drawList = nullptr;
    bool m_bTableActive = false;

    void SetChannelForeground() const;
    void SetChannelBackground() const;

    struct BgState {
        ImVec2 hMin{};
        ImVec2 hMax{};
        float rnd = 0.0f;
    } m_bgState;
};

/**
 * This class is responsible for rendering the UI using ImGui, including initialization
 * destruction, and event handling.
 */
class UIRenderer {
public:
    explicit UIRenderer(const GfxContext &gfx)
        : m_gfx(gfx) {}

    void Init(
        const std::function<void()> &importSceneCallback,
        const std::function<void()> &exportSceneCallback,
        const std::function<void()> &loadHDRICallback,
        const std::function<void()> &renderImageCallback);
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
    void NewFrame(SceneUpdate &update);

    void NewFrameRender() const;

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

    void RegisterViewportBackend(ViewportBackend id, const char *name, const std::function<void(UiDrawContext &)> &settings);
    void RegisterRenderBackend(RenderBackend id, const char *name, const std::function<void(UiDrawContext &)> &settings);

    void LoadScene(Scene *scene);
private:
    const GfxContext &m_gfx;
    VkDescriptorPool m_descriptorPool{};

    Scene *m_pScene{};
    std::vector<std::string> objects{};

    // We need to store the central node so we can easily retrieve its dimensions
    ImGuiDockNode *m_pCentralNode{};
    bool m_bStartupFocusSet = false;

    void SetupStyle() const;

    // Docking setup
    static void SetLayout(ImGuiID dockSpaceId, const ImGuiViewport *viewport, ImGuiDockNodeFlags dockSpaceFlags);

    // Backend Registry
    // These are indexed by their ID (enum value)
    const char *m_viewportBackendNames[JTX_NUM_VIEWPORT_BACKENDS]{};
    std::function<void(UiDrawContext &)> m_viewportBackendSettings[JTX_NUM_VIEWPORT_BACKENDS]{};

    const char *m_renderBackendNames[JTX_NUM_RENDER_BACKENDS]{};
    std::function<void(UiDrawContext &)> m_renderBackendSettings[JTX_NUM_RENDER_BACKENDS]{};

    int m_currentRenderBackend   = 0;
    int m_currentViewportBackend = 0;

    std::function<void()> m_importSceneCallback;
    std::function<void()> m_exportSceneCallback;
    std::function<void()> m_loadHDRICallback;
    std::function<void()> m_renderImageCallback;
    bool m_bRenderWindowOpen = false;
};

}// namespace jtx
