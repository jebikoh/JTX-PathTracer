#pragma once

#include <editor/gfx_context.hpp>
#include <engine/vulkan/queue.hpp>

namespace jtx {

/**
 * Render thread, owns all GPU resources
 */
struct RenderSystem {
    void Init();
    void Destroy();

private:
    GfxContext &m_ctx;
};

}// namespace jtx