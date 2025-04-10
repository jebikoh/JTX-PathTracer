#pragma once

#include "jvk/commands.hpp"
#include "jvk/context.hpp"
#include "jvk/descriptor.hpp"
#include "jvk/fence.hpp"
#include "jvk/image.hpp"
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
 * operate on them.
 */
class Display {
public:
    void init();
    void draw();
    void run();
    void cleanup();

    static Display *get();

private:
    bool m_isInitialized = false;
    bool m_stopRendering = false;
    int m_frameNumber = 0;
    float m_deltaTime = 1;

    VkExtent2D m_extent{1920, 1080};

    // Vulkan & allocators
    jvk::Context m_context;
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
    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initDescriptors();
    void initDrawImages();
    void initFrameData();

    // Default data
    void initDefaultImages();
    void initDefaultSamplers();
};