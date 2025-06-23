#include "nfd.h"


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
    m_uiRenderer.Init();
    m_wing.Init(m_gfx.bRayTracingSupported);

    m_uiRenderer.RegisterViewportBackend(JTX_VIEWPORT_BACKEND_WING, "Wing",
                                         [this](UiDrawContext &ctx) {
                                             m_wing.DrawSettingsPanel(ctx);
                                         });

    LOG_INFO(DISPLAY, "Display initialized");
}

void Display::Destroy() {
    LOG_INFO(DISPLAY, "Destroying display");

    m_gfx.WaitIdle();
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

}// namespace jtx
