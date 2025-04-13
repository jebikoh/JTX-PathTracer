#pragma once

#include "jvk/jvk.hpp"
#include "jvk/buffer.hpp"

namespace jtx {

class Display;

/**
 * This class is responsible for rasterizing the scene, including managing
 * GPU resources, pipelines, and rendering commands.
 */
class Rasterizer {
public:
    Rasterizer(Display *pDisplay)
        : m_pDisplay(pDisplay) {}

    void destroy();

    /**
     * This will (re)load the scene from the display and setup GPU resources.
     * If a scene was previously loaded, it's corresponding GPU resources will be freed prior.
     */
    void loadScene();

private:
    Display *m_pDisplay = nullptr;
    bool m_bSceneLoaded = false;

    struct GPUSceneBuffers {
        jvk::Buffer index;
        // Vertex buffers
        jvk::Buffer position;
        VkDeviceAddress positionAddress;
        jvk::Buffer normal;
        VkDeviceAddress normalAddress;
        jvk::Buffer uv;
        VkDeviceAddress uvAddress;
        jvk::Buffer color;
        VkDeviceAddress colorAddress;
    } m_gpuSceneBuffers;
    void clearGPUSceneBuffers();
};

}// namespace jtx
