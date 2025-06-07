#pragma once

#include <ui/gfx_context.hpp>
#include <ui/ui_renderer.hpp>
#include <ui/rasterizer/rasterizer.hpp>
#include <scene/scene.hpp>

namespace jtx {

/**
 * This class is responsible for the UI and handling high-level rendering
 * and state-management
 *
 * It consists of three sub-renderers:
 *  - Rasterizer:  draws the scene using rasterized PBR.
 *  - UI renderer: draws the UI with ImGui and handles state updates
 *  - PT renderer: dispatches command to selected PT (path-tracing) backend and
 *                 renders the progressive output to a textured quad.
 *
 * All the high-level Vulkan structures are stored in this class and the sub-renderers
 * operate on them. This only for logical organization and readability.
 */
class Display {
public:
    Display() : m_gfx(), m_uiRenderer(m_gfx), m_rasterizer(m_gfx) {};

    void Init();
    void Destroy();

    void Draw();
    void Run();

    void SetScene(jtx::Scene *pScene) {
        m_pScene = pScene;
        m_rasterizer.LoadScene(pScene);
    }
private:
    bool m_bIsInitialized   = false;
    bool m_bStopRendering   = false;
    jtx::Scene *m_pScene = nullptr;

    GfxContext m_gfx;
    UIRenderer m_uiRenderer;
    Rasterizer m_rasterizer;
};

}// namespace jtx
