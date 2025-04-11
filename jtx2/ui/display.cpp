#include "display.hpp"

#include "jvk/util.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <imgui_impl_sdl2.h>
#include <thread>

constexpr bool JTX_USE_VALIDATION_LAYERS = true;

Display *loadedDisplay = nullptr;

#pragma region Initialization

void Display::init() {
    LOG_INFO(DISPLAY, "Initializing display");

    assert(loadedDisplay == nullptr);
    loadedDisplay = this;

    initWindow();
    initVulkan();
    initAllocators();
    initSwapchain();
    initDrawImages();
    initFrameData();
    initImmediateBuffer();


    m_bIsInitialized = true;
    LOG_INFO(DISPLAY, "Initialized display");
}
void Display::run() {
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

            // ImGui_ImplSDL2_ProcessEvent(&e);
        }

        if (m_bStopRendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (m_bResizeRequested) resizeSwapchain();
        // draw() -> ui renderer
        // draw() -> rasterizer or RT output
    }
}

void Display::cleanup() {
    LOG_INFO(DISPLAY, "Cleaning up engine resoures...");
    destroyImmediateBuffer();
    destroyFrameData();
    destroyDrawImages();
    destroySwapchain();
    destroyAllocators();
    destroyVulkan();
    destroyWindow();
    LOG_INFO(DISPLAY, "Engine resources cleared");
}

Display *Display::get() {
    return loadedDisplay;
}

void Display::initWindow() {
    LOG_INFO(DISPLAY, "Initializing window");
    SDL_Init(SDL_INIT_VIDEO);
    constexpr auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    m_pWindow                  = SDL_CreateWindow(
            "JTX",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            static_cast<int>(m_windowExtent.width),
            static_cast<int>(m_windowExtent.height),
            windowFlags);
    LOG_INFO(DISPLAY, "Window initialized");
}

void Display::initVulkan() {
    LOG_INFO(DISPLAY, "Initializing vulkan");

    // Volk
    const auto volkResult = volkInitialize();
    if (volkResult != VK_SUCCESS) {
        LOG_FATAL(DISPLAY, "Failed to initialize volk");
    }

    // Vulkan instance
    vkb::InstanceBuilder instanceBuilder;
    const auto vkbInstanceResult = instanceBuilder
                                           .set_app_name("JTX")
                                           .request_validation_layers(JTX_USE_VALIDATION_LAYERS)
                                           .use_default_debug_messenger()
                                           .require_api_version(1, 2, 0)
                                           .build();
    if (!vkbInstanceResult) {
        LOG_FATAL(DISPLAY, "Failed to initialize vkb instance");
    }
    const vkb::Instance vkbInstance = vkbInstanceResult.value();
    m_ctx.instance                  = vkbInstance.instance;
    m_ctx.debugMessenger            = vkbInstance.debug_messenger;
    volkLoadInstance(m_ctx.instance);

    // SDL surface
    SDL_Vulkan_CreateSurface(m_pWindow, m_ctx, &m_ctx.surface);

    // Vulkan physical device
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing  = true;

    vkb::PhysicalDeviceSelector pdSelector{vkbInstance};
    const auto vkbPdResult = pdSelector
                                     .set_minimum_version(1, 2)
                                     .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                                     .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                                     .add_required_extension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME)
                                     .set_required_features_12(features12)
                                     .set_surface(m_ctx)
                                     .select();
    if (!vkbPdResult) {
        LOG_FATAL(DISPLAY, "Failed to initialize vkb physical device");
    }
    const vkb::PhysicalDevice vkbPd = vkbPdResult.value();
    m_ctx.physicalDevice            = vkbPd.physical_device;

    // Vulkan device
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{};
    dynamicRendering.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRendering.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2{};
    synchronization2.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2.synchronization2 = VK_TRUE;
    synchronization2.pNext            = &dynamicRendering;

    vkb::DeviceBuilder deviceBuilder{vkbPd};
    deviceBuilder.add_pNext(&synchronization2);
    vkb::Device vkbDevice = deviceBuilder.build().value();
    m_ctx.device          = vkbDevice;

    volkLoadDevice(m_ctx.device);

    // Graphics queue
    m_graphicsQueue.queue  = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_graphicsQueue.family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    LOG_INFO(DISPLAY, "Vulkan initialized");
}

void Display::initAllocators() {
    LOG_INFO(DISPLAY, "Initializing allocators");
    // VMA
    VmaVulkanFunctions vkFunctions{};
    vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vkFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice   = m_ctx;
    allocatorInfo.device           = m_ctx;
    allocatorInfo.instance         = m_ctx;
    allocatorInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.pVulkanFunctions = &vkFunctions;
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    // Global descriptor allocator
    std::vector<jvk::DynamicDescriptorAllocator::PoolSizeRatio> sizes =
            {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};

    m_descriptorAllocator.init(m_ctx, 10, sizes);
    LOG_INFO(DISPLAY, "Allocators initialized");
}

void Display::initSwapchain() {
    LOG_INFO(DISPLAY, "Initializing swapchain");
    m_swapchain.init(m_ctx, m_windowExtent.width, m_windowExtent.height);
    LOG_INFO(DISPLAY, "Swapchain initialized");
}

void Display::initDrawImages() {
    LOG_INFO(DISPLAY, "Initializing draw image");
    // Draw image
    VkExtent3D drawExtent{};
    drawExtent.width  = m_windowExtent.width;
    drawExtent.height = m_windowExtent.height;
    drawExtent.depth  = 1;

    m_drawImage.image.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_drawImage.image.imageExtent = drawExtent;

    VkImageUsageFlags drawImageUsages = {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;    // Copy from image
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;    // Copy to image
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;         // Allow compute shader to write
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// Graphics pipeline

    const VkImageCreateInfo drawImageInfo = jvk::init::image(m_drawImage.image.imageFormat, drawImageUsages, m_drawImage.image.imageExtent);

    VmaAllocationCreateInfo drawImageAllocInfo{};
    drawImageAllocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(m_allocator, &drawImageInfo, &drawImageAllocInfo, &m_drawImage.image.image, &m_drawImage.image.allocation, nullptr);

    const VkImageViewCreateInfo imageViewInfo = jvk::init::imageView(m_drawImage.image.imageFormat, m_drawImage.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(m_ctx, &imageViewInfo, nullptr, &m_drawImage.image.imageView));

    // Depth/stencil image
    jvk::getSupportedDepthStencilFormat(m_ctx, &m_drawImage.depthStencilImage.imageFormat);
    m_drawImage.depthStencilImage.imageExtent = drawExtent;

    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo depthImageInfo = jvk::init::image(m_drawImage.depthStencilImage.imageFormat, depthImageUsages, drawExtent);
    vmaCreateImage(m_allocator, &depthImageInfo, &drawImageAllocInfo, &m_drawImage.depthStencilImage.image, &m_drawImage.depthStencilImage.allocation, nullptr);

    VkImageViewCreateInfo depthImageViewInfo = jvk::init::imageView(m_drawImage.depthStencilImage.imageFormat, m_drawImage.depthStencilImage.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    CHECK_VK(vkCreateImageView(m_ctx, &depthImageViewInfo, nullptr, &m_drawImage.depthStencilImage.imageView));
    LOG_INFO(DISPLAY, "Draw image initialized");
}

void Display::initFrameData() {
    constexpr VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    for (auto &frame: m_frameData) {
        // Command pools
        CHECK_VK(frame.cmdPool.init(m_ctx, m_graphicsQueue.family, flags));
        CHECK_VK(frame.cmdPool.allocateCommandBuffer(&frame.cmdBuffer));

        // Synchronization
        CHECK_VK(frame.drawFence.init(m_ctx, VK_FENCE_CREATE_SIGNALED_BIT));
        CHECK_VK(frame.swapchainSemaphore.init(m_ctx));
        CHECK_VK(frame.drawSemaphore.init(m_ctx));
    }
}

void Display::initImmediateBuffer() {
    LOG_INFO(DISPLAY, "Initializing immediate buffer");
    CHECK_VK(m_immBuffer.init(m_ctx, m_graphicsQueue.family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
    LOG_INFO(DISPLAY, "Immediate buffer initialized");
}

#pragma endregion
#pragma region Cleanup

void Display::destroyWindow() const {
    LOG_INFO(DISPLAY, "Destroying window");
    SDL_DestroyWindow(m_pWindow);
    LOG_INFO(DISPLAY, "Window destroyed");
}

void Display::destroyVulkan() {
    LOG_INFO(DISPLAY, "Destroying Vulkan context");
    m_ctx.destroy();
    LOG_INFO(DISPLAY, "Vulkan context destroyed");
}

void Display::destroyAllocators() {
    LOG_INFO(DISPLAY, "Destroying allocators");
    m_descriptorAllocator.destroyPools(m_ctx);
    vmaDestroyAllocator(m_allocator);
    LOG_INFO(DISPLAY, "Allocators destroyed");
}

void Display::destroySwapchain() {
    LOG_INFO(DISPLAY, "Destroying swapchain");
    m_swapchain.destroy(m_ctx);
    LOG_INFO(DISPLAY, "Swapchain destroyed");
}

void Display::destroyDrawImages() const {
    LOG_INFO(DISPLAY, "Destroying draw images");
    m_drawImage.image.destroy(m_ctx, m_allocator);
    m_drawImage.depthStencilImage.destroy(m_ctx, m_allocator);
    LOG_INFO(DISPLAY, "Draw images destroyed");
}

void Display::destroyFrameData() {
    LOG_INFO(DISPLAY, "Destroying frame data");
    for (auto &frame: m_frameData) {
        frame.drawSemaphore.destroy();
        frame.swapchainSemaphore.destroy();
        frame.drawFence.destroy();

        frame.cmdPool.destroy();
    }
    LOG_INFO(DISPLAY, "Frame data destroyed");
}

void Display::destroyImmediateBuffer() {
    LOG_INFO(DISPLAY, "Destroying immediate buffer");
    m_immBuffer.destroy();
    LOG_INFO(DISPLAY, "Immediate buffer destroyed");
}

#pragma endregion

void Display::resizeSwapchain() {
    LOG_INFO(DISPLAY, "Resizing swapchain");
    vkDeviceWaitIdle(m_ctx);
    m_swapchain.destroy(m_ctx);

    int w, h;
    SDL_GetWindowSize(m_pWindow, &w, &h);
    m_windowExtent.width = w;
    m_windowExtent.height = h;

    m_swapchain.init(m_ctx, m_windowExtent.width, m_windowExtent.height);
    m_bResizeRequested = false;
    LOG_INFO(DISPLAY, "Swapchain resized");
}
