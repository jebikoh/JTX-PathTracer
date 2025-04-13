#pragma once

#include "jvk/buffer.hpp"
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

#include "ui_renderer.hpp"

constexpr int JTX_MAX_FRAMES_IN_FLIGHT = 2;

class RasterizationEngine;

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
    // Needs access to createImage/createBuffer methods
    friend class RasterizationEngine;
    friend class UIRenderer;

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

    VkExtent2D m_windowExtent{1700, 900};
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

    // Secondary renderers
    UIRenderer m_uiRenderer{this};

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
    void destroyDefaultImages() const;
    void destroyDefaultSamplers() const;

    // Image utilities
    // The caller is responsible for keeping track of these and destroying them on cleanup

    /**
     * Creates an empty image with the given parameters using this engine's allocator.
     *
     * The user is responsible for keeping track of and destroying this image on cleanup.
     * @param extent image extent
     * @param format image format
     * @param usage memory usage
     * @param bMipmapped true if the image should be mipmapped
     * @param sampleCount sample count
     * @return empty image
     */
    jvk::Image createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool bMipmapped = false, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) const;

    /**
     * Creates an image with the given parameters using this engine's allocator.
     * Copies the provided data to the image via a staging buffer
     *
     *
     * The user is responsible for keeping track of and destroying this image on cleanup.
     * @param pData data pointer
     * @param extent data extent
     * @param nChannels data channel count (TODO: have this be auto-detected from the format)
     * @param format image format
     * @param usage image memory usage
     * @param bMipmapped true if the image should be mipmapped
     * @param sampleCount sample count
     * @return image containing provided data
     */
    jvk::Image createImage(const void *pData, VkExtent3D extent, size_t nChannels, VkFormat format, VkImageUsageFlags usage, bool bMipmapped = false, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) const;

    /**
     * Destroys the image using this engine's context and allocator
     * @param image image to destroy
     */
    void destroyImage(const jvk::Image &image) const;

    // Buffer utilities

    /**
     * Creates an empty mapped buffer with the given parameters using this engine's allocator.
     * @param allocSize size of the buffer
     * @param usage buffer usage
     * @param memUsage memory usage
     * @return empty buffer
     */
    jvk::Buffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) const;

    /**
     * Destroys the buffer using this engine's allocator
     * @param buffer buffer to destroy
     */
    void destroyBuffer(const jvk::Buffer &buffer) const;

    void resizeSwapchain();
};