#include "nfd.h"
#include "scene/scene_loader.hpp"


#include <SDL_events.h>
#include <interface/display.hpp>
#include <thread>

namespace jtx {

Display *pLoadedDisplay = nullptr;

void Display::Init() {
    LOG_INFO(DISPLAY, "Initializing display");

    assert(pLoadedDisplay == nullptr);
    pLoadedDisplay = this;

    m_gfx.Init();
    m_uiRenderer.Init([this] {
        ImportScene();
    });
    m_wing.Init(m_gfx.bRayTracingSupported);

    m_uiRenderer.RegisterViewportBackend(JTX_VIEWPORT_BACKEND_WING, "Wing",
                                         [this](UiDrawContext &ctx) {
                                             m_wing.DrawSettingsPanel(ctx);
                                         });

    if (NFD_Init() != NFD_OKAY) {
        LOG_FATAL(DISPLAY, "Failed to initialize NFD");
    }

    LOG_INFO(DISPLAY, "Display initialized");
}

void Display::Destroy() {
    LOG_INFO(DISPLAY, "Destroying display");

    m_gfx.WaitIdle();


    NFD_Quit();
    if (m_bSceneLoaded) {
        m_Scene.Destroy();
    }

    m_wing.Destroy();
    m_uiRenderer.Destroy();
    m_gfx.Destroy();
    pLoadedDisplay = nullptr;

    LOG_INFO(DISPLAY, "Display destroyed");
}

void Display::Draw() {
    m_uiRenderer.NewFrame();

    auto res = m_gfx.StartFrame();
    if (!res.has_value()) return;
    auto &ctx = res.value();

    jvk::ViewRectangle rect;
    if (m_uiRenderer.GetViewportRectangle(rect)) {
        m_wing.SetViewportRectangle(rect);
    }

    ResolveRegion region;
    m_wing.Draw(ctx, region);
    m_gfx.ResolveToSwapchain(ctx, region);

    m_uiRenderer.Draw(ctx);

    m_gfx.EndFrame(ctx);
}

void Display::Run() {
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    m_bStopRendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    m_bStopRendering = false;
                }
            }

            if (m_uiRenderer.ProcessEvent(e)) {
                m_wing.SkipEvent();
            } else {
                m_wing.ProcessEvent(e);
            }
        }

        if (m_bStopRendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        m_gfx.ResizeSwapchain();

        Draw();
    }
}

void Display::ImportScene() {
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
            m_Scene.Destroy();
        }

        const auto loadResult = LoadScene(s, m_Scene);
        if (loadResult > 0) {
            m_wing.LoadScene(&m_Scene);
        }
    } else if (result == NFD_CANCEL) {
        LOG_DEBUG(UI, "User cancelled scene import");
    } else {
        LOG_ERROR(UI, "Error while opening file dialog: {}", NFD_GetError());
    }
}

}// namespace jtx
