#pragma once

#include <editor/gfx_context.hpp>
#include <editor/ui_renderer.hpp>
#include <engine/vulkan/vk_engine.hpp>
#include <scene/scene.hpp>

namespace jtx {

/**
 * This class is responsible for the UI and high-level rendering logic of the editor.
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
class Editor {
public:
    Editor() : m_gfx(), m_ui(m_gfx), m_vk(m_gfx) {};

    void Init();
    // Loads scene during initialization
    // Primary meant for debugging -- will abort if scene load is unsuccessful
    void Init(const std::filesystem::path &path);

    void Destroy();

    void Draw();
    void Run();
private:
    bool m_bIsInitialized   = false;
    bool m_bStopRendering   = false;

    jtx::Scene m_scene;
    bool m_bSceneLoaded = false;

    GfxContext m_gfx;
    UIRenderer m_ui;
    VkEngine m_vk;

    void ImportScene();
    void ExportScene() const;

    void LoadHDRI();
};

}// namespace jtx
