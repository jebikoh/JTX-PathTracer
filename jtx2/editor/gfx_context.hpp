#pragma once
#include <jvk/buffer.hpp>
#include <jvk/commands.hpp>
#include <jvk/context.hpp>
#include <jvk/fence.hpp>
#include <jvk/image.hpp>
#include <jvk/immediate_buffer.hpp>
#include <jvk/jvk.hpp>
#include <jvk/queue.hpp>
#include <jvk/sampler.hpp>
#include <jvk/semaphore.hpp>
#include <jvk/swapchain.hpp>

struct SDL_Window;

namespace jtx {

struct Window {
    VkExtent2D extent{800, 400};
    SDL_Window *pWindow   = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    jvk::Swapchain swapchain;
    uint32_t id;
};

enum class kRenderTarget {
    DRAW16f,
    DRAW32f,
    DEPTH_STENCIL
};

struct RenderContext {
    const jvk::CommandBuffer cmd;

    const uint32_t swapchainIndex = 0;
    const uint32_t frameIndex     = 0;

    const struct SwapchainContext {
        VkImage image     = VK_NULL_HANDLE;
        VkImageView view  = VK_NULL_HANDLE;
        VkExtent2D extent = {0, 0};
    } swapchain;

    struct LayoutState {
        VkImageLayout swapchain    = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout draw16f      = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout draw32f      = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout depthStencil = VK_IMAGE_LAYOUT_UNDEFINED;
    } layout;
};

struct ResolveRegion {
    VkExtent2D src[2];
    VkExtent2D dst[2];
    kRenderTarget target;
};

/**
 * This class is responsible for maintain the Vulkan rendering context.
 *
 * All the high-level Vulkan structures are stored in this class and is passed around to subsystems by reference.
 *
 * TODO: Need to refactor this to either separate the window portion or have it be optional.
 *       Want to be able to use this for offline rendering without a window/swapchain
 *       I am thinking that maybe we can pass in a config struct to determine how to initialize the context
 */
struct GfxContext {
    struct MainWindow {
        VkExtent2D extent{1700, 900};
        SDL_Window *pWindow = nullptr;
        uint32_t id;
    } window;

    jvk::VkContext ctx;

    bool bRayTracingSupported = true;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

    VmaAllocator allocator;

    int frameNumber = 0;
    float deltaTime = 1;
    struct FrameData {
        jvk::CommandPool cmdPool;
        jvk::CommandBuffer cmdBuffer;

        jvk::Semaphore imageAvailableSemaphore;
        jvk::Fence drawFence;
    } frameData[JTX_MAX_FRAMES_IN_FLIGHT];

    jvk::Swapchain swapchain;
    std::vector<jvk::Semaphore> renderFinishedSemaphores;

    jvk::Queue graphicsQueue;
    jvk::ImmediateBuffer imBuffer;

    // draw32f only initialized if ray tracing is available
    struct RenderTarget {
        jvk::Image draw16f;
        jvk::Image draw32f;
        jvk::Image depthStencil;
    } targets;

    struct DefaultImages {
        jvk::Image white;
        jvk::Image black;
        jvk::Image checkerboard;
    } defaultImages;

    struct DefaultSamplers {
        jvk::Sampler linear;
        jvk::Sampler nearest;
    } defaultSamplers;

    void Init();
    void Destroy();

    std::optional<RenderContext> StartFrame();
    void EndFrame(const RenderContext &renderCtx);

    /**
     * Copies the contents of the draw image to the swapchain image.
     * @param renderCtx render context
     * @param region copy regions
     */
    void ResolveToSwapchain(RenderContext &renderCtx, const ResolveRegion &region) const;

    void WaitIdle() const { vkDeviceWaitIdle(ctx); }

    FrameData &GetCurrentFrame() { return frameData[frameNumber % JTX_MAX_FRAMES_IN_FLIGHT]; }
    size_t GetCurrentFrameIndex() const { return frameNumber % JTX_MAX_FRAMES_IN_FLIGHT; }

    /**
     * Resizes the swapchain to match the current window size if out of date.
     */
    void ResizeSwapchain();

    /**
     * Creates an empty image with the given parameters using this engine's allocator.
     *
     * The user is responsible for keeping track of and destroying this image on destroy.
     * @param extent image extent
     * @param format image format
     * @param usage memory usage
     * @param bMipmapped true if the image should be mipmapped
     * @param sampleCount sample count
     * @return empty image
     */
    jvk::Image CreateImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool bMipmapped = false, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) const;

    /**
     * Creates an image with the given parameters using this engine's allocator.
     * Copies the provided data to the image via a staging buffer.
     *
     *
     * The user is responsible for keeping track of and destroying this image on destroy.
     * @param pData data pointer
     * @param extent data extent
     * @param nChannels data channel count (TODO: have this be auto-detected from the format)
     * @param format image format
     * @param usage image memory usage
     * @param bytesPerPixel bytes per single pixel
     * @param bMipmapped true if the image should be mipmapped
     * @param sampleCount sample count
     * @return image containing provided data
     */
    jvk::Image CreateImage(const void *pData, VkExtent3D extent, size_t nChannels, VkFormat format, VkImageUsageFlags usage, size_t bytesPerPixel = 1, bool bMipmapped = false, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) const;

    /**
     * Destroys the image using this engine's context and allocator
     * @param image image to destroy
     */
    void DestroyImage(const jvk::Image &image) const;

    /**
     * Creates an empty buffer with the given parameters using this engine's allocator.
     * @param allocSize size of the buffer
     * @param usage buffer usage
     * @param memUsage memory usage
     * @param memFlags memory flags
     * @param memPropFlags vulkan memory flags
     * @param minAlignment minimum alignment
     * @return empty buffer
     */
    jvk::Buffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags memFlags = 0, VkMemoryPropertyFlags memPropFlags = 0, VkDeviceSize minAlignment = 0) const;

    /**
     * Destroys the buffer using this engine's allocator
     * @param buffer buffer to destroy
     */
    void DestroyBuffer(jvk::Buffer &buffer) const;

    /**
     * Notifies the GFX context that the swapchain will be out of date
     *
     * Can be used to proactively resize the swapchain before an aquire/present error
     */
    void NotifyResize() { m_bSwapchainOutOfDate = true; }

    /**
     * Will create an external window with this GFX context
     * @param extent window extent
     * @param out window to initialize
     */
    void CreateExternalWindow(VkExtent2D extent, Window &out) const;

    /**
     * Destroys an external window that was created with this GFX context
     * @param window window to destroy
     */
    void DestroyExternalWindow(Window &window) const;

private:
    bool m_bSwapchainOutOfDate = false;

    void InitWindow();
    void InitVulkan();
    void InitAllocator();
    void InitSwapchain();
    void InitRenderTarget();
    void InitFrameData();
    void InitImmediateBuffer();
    void InitDefaultImages();
    void InitDefaultSamplers();

    void DestroyWindow() const;
    void DestroyVulkan() const;
    void DestroyAllocator() const;
    void DestroySwapchain() const;
    void DestroyRenderTarget() const;
    void DestroyFrameData();
    void DestroyImmediateBuffer() const;
    void DestroyDefaultImages() const;
    void DestroyDefaultSamplers() const;
};

}// namespace jtx