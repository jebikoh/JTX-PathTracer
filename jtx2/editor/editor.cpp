#include "scene/scene_loader.hpp"
#include <nfd.h>

#include <SDL3/SDL_events.h>
#include <editor/editor.hpp>
#include <scene/scene_exporter.hpp>
#include <thread>

#include <util/profiling.hpp>

namespace jtx {

Editor *pLoadedDisplay = nullptr;

void Editor::Init() {
    TPROFILE_SCOPE();

    LOG_INFO(DISPLAY, "Initializing display");

    assert(pLoadedDisplay == nullptr);
    pLoadedDisplay = this;

    m_gfx.Init();
    m_ui.Init([this] { ImportScene(); },
              [this] {
                  ExportScene();
              },
              [this] {
                  LoadHDRI();
              },
              [this] {
                  RenderImage();
              });
    m_vk.Init(m_gfx.bRayTracingSupported);

    m_ui.RegisterRenderBackend(JTX_RENDER_BACKEND_VULKAN, "Vulkan",
        [this](UiDrawContext &ctx) {
                        m_vk.DrawRenderSettingsPanel(ctx);
                    });

    m_ui.RegisterViewportBackend(JTX_VIEWPORT_BACKEND_VULKAN, "Vulkan",
                                 [this](UiDrawContext &ctx) {
                                     m_vk.DrawViewportSettingsPanel(ctx);
                                 });

    if (NFD_Init() != NFD_OKAY) {
        LOG_FATAL(DISPLAY, "Failed to initialize NFD");
    }

    LOG_INFO(DISPLAY, "Display initialized");
}

void Editor::Init(const std::filesystem::path &path) {
    TPROFILE_SCOPE();
    CHECK_JTX(LoadScene(path, m_scene));
    Init();
    m_vk.LoadScene(&m_scene);
    m_ui.LoadScene(&m_scene);
    m_bSceneLoaded = true;
}

void Editor::Destroy() {
    TPROFILE_SCOPE();
    LOG_INFO(DISPLAY, "Destroying display");

    m_gfx.WaitIdle();


    NFD_Quit();
    if (m_bSceneLoaded) {
        m_scene.Destroy();
    }

    m_vk.Destroy();
    m_ui.Destroy();
    m_gfx.Destroy();
    pLoadedDisplay = nullptr;

    LOG_INFO(DISPLAY, "Display destroyed");
}

void Editor::Draw() {
    TPROFILE_SCOPE();

    SceneUpdate update;
    m_ui.NewFrame(update);

    auto res = m_gfx.StartFrame();
    if (!res.has_value()) return;
    auto &ctx = res.value();

    jvk::ViewRectangle rect;
    if (m_ui.GetViewportRectangle(rect) && rect.Area() > 0) {
        m_vk.SetViewportRectangle(rect);

        ResolveRegion region;
        m_vk.Draw(ctx, region, update);
        m_gfx.ResolveToSwapchain(ctx, region);
    }

    m_ui.Draw(ctx);

    m_gfx.EndFrame(ctx);
}

void Editor::Run() {
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit) {

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) bQuit = true;

            if (e.type >= 0x202 && e.type < 0x300) {

                if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                    bQuit = true;
                }

                if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                    m_gfx.NotifyResize();
                }

                if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                    m_bStopRendering = true;
                }

                if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                    m_bStopRendering = false;
                }
            }

            if (m_ui.ProcessEvent(e)) {
                m_vk.SkipEvent();
            } else {
                m_vk.ProcessEvent(e);
            }
        }

        if (m_bStopRendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        m_gfx.ResizeSwapchain();

        Draw();

        TPROFILE_FRAME_MARK();
    }
}

void Editor::ImportScene() {
    TPROFILE_SCOPE();
    constexpr nfdu8filteritem_t filters[1] = {{"Scene file", "jtx,obj,gltf,glb"}};
    nfdopendialogu8args_t args{};
    args.filterList  = filters;
    args.filterCount = 1;

    nfdu8char_t *outPath;
    const nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

    if (result == NFD_OKAY) {
        const auto s = std::string(outPath);
        LOG_INFO(UI, "User selected path: {}", outPath);
        NFD_FreePathU8(outPath);

        m_gfx.WaitIdle();
        if (m_bSceneLoaded) {
            m_scene.Destroy();
        }

        const auto loadResult = LoadScene(s, m_scene);
        if (loadResult > 0) {
            m_vk.LoadScene(&m_scene);
            m_ui.LoadScene(&m_scene);
            m_bSceneLoaded = true;
        } else {
            m_bSceneLoaded = false;
        }
    } else if (result == NFD_CANCEL) {
        LOG_DEBUG(UI, "User cancelled scene import");
    } else {
        LOG_ERROR(UI, "Error while opening file dialog: {}", NFD_GetError());
    }
}

void Editor::ExportScene() const {
    TPROFILE_SCOPE();
    const auto name = m_scene.name.empty() ? "scene.jtx" : m_scene.name + ".jtx";

    constexpr nfdu8filteritem_t filters[1] = {{"JTX scene file", "jtx"}};
    nfdsavedialogu8args_t args{};
    args.filterList  = filters;
    args.filterCount = 1;
    args.defaultName = name.c_str();

    nfdu8char_t *outPath;
    const nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY) {
        const auto s = std::string(outPath);
        LOG_INFO(UI, "User selected export path: {}", outPath);
        NFD_FreePathU8(outPath);

        const auto exportResult = jtx::ExportScene(m_scene, s);
    } else if (result == NFD_CANCEL) {
        LOG_DEBUG(UI, "User cancelled scene export");
    } else {
        LOG_ERROR(UI, "Error while opening save dialog: {}", NFD_GetError());
    }
}

void Editor::LoadHDRI() {
    TPROFILE_SCOPE();

    constexpr nfdu8filteritem_t filters[1] = {{"HDR image", "hdr,exr"}};
    nfdopendialogu8args_t args{};
    args.filterList  = filters;
    args.filterCount = 1;

    nfdu8char_t *imgPath;
    const nfdresult_t result = NFD_OpenDialogU8_With(&imgPath, &args);

    if (result == NFD_OKAY) {
        const auto s = std::string(imgPath);
        LOG_INFO(UI, "User selected path: {}", imgPath);
        NFD_FreePathU8(imgPath);

        m_scene.envmap.image.Destroy();
        const auto res = Image32f::Load(s, m_scene.envmap.image);
        if (res > 0) {
            m_vk.LoadHDRI();
        }
    } else if (result == NFD_CANCEL) {
        LOG_DEBUG(UI, "User cancelled envmap import");
    } else {
        LOG_ERROR(UI, "Error while opening file dialog: {}", NFD_GetError());
    }
}

void Editor::RenderImage() {
    TPROFILE_SCOPE();

    m_activeWindow = RENDER;
}

}// namespace jtx
