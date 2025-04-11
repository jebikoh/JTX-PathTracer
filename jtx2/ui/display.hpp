#pragma once

#include "jvk/commands.hpp"
#include "jvk/context.hpp"
#include "jvk/descriptor.hpp"
#include "jvk/fence.hpp"
#include "jvk/image.hpp"
#include "jvk/immediate_buffer.hpp"
#include "jvk/jvk.hpp"
#include "jvk/queue.hpp"
#include "jvk/sampler.hpp"
#include "jvk/semaphore.hpp"
#include "jvk/swapchain.hpp"

constexpr int JTX_MAX_FRAMES_IN_FLIGHT = 2;

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
 * operate on them. This only for purposes of logical organization.
 */
class Display {
public:
    void init();
    void draw();
    void run();
    void cleanup();

    static Display *get();

private:
    bool m_bIsInitialized   = false;
    bool m_bStopRendering   = false;
    bool m_bResizeRequested = false;
    int m_frameNumber       = 0;
    float m_deltaTime       = 1;

    VkExtent2D m_windowExtent{1920, 1080};
    struct SDL_Window *m_pWindow = nullptr;

    // Vulkan & allocators
    jvk::Context m_ctx;
    VmaAllocator m_allocator{};
    jvk::DynamicDescriptorAllocator m_descriptorAllocator;

    // Frames
    jvk::Swapchain m_swapchain;
    struct FrameData {
        jvk::CommandPool cmdPool;
        jvk::CommandBuffer cmdBuffer;

        jvk::Semaphore swapchainSemaphore;
        jvk::Semaphore drawSemaphore;
        jvk::Fence drawFence;
    } m_frameData[JTX_MAX_FRAMES_IN_FLIGHT];

    // Queues
    jvk::Queue m_graphicsQueue;
    jvk::ImmediateBuffer m_immBuffer;

    // Draw Images (Rasterization)
    struct DrawImage {
        jvk::Image image;
        jvk::Image depthStencilImage;
        VkExtent2D extent;
        float renderScale = 1.0f;
    } m_drawImage{};

    // Default Textures & Samplers
    jvk::Image m_whiteImage{};
    jvk::Image m_blackImage{};
    jvk::Image m_errorCheckerboardImage{};

    jvk::Sampler m_samplerLinear;
    jvk::Sampler m_samplerNearest;

    // Functions
    FrameData &getCurrentFrame() { return m_frameData[m_frameNumber % JTX_MAX_FRAMES_IN_FLIGHT]; }

    // Initializer functions
    void initWindow();
    void initVulkan();
    void initAllocators();
    void initSwapchain();
    void initDrawImages();
    void initFrameData();
    void initImmediateBuffer();

    void destroyWindow() const;
    void destroyVulkan();
    void destroyAllocators();
    void destroySwapchain();
    void destroyDrawImages() const;
    void destroyFrameData();
    void destroyImmediateBuffer();

    // Default data
    void initDefaultImages();
    void initDefaultSamplers();

    void resizeSwapchain();
};