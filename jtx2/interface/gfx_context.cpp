#define VOLK_IMPLEMENTATION
#include <interface/gfx_context.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <jvk/util.hpp>

constexpr bool JTX_USE_VALIDATION_LAYERS = true;

namespace jtx {

#pragma region Initialization

void GfxContext::Init() {
    LOG_INFO(DISPLAY, "Initializing GFX context");

    InitWindow();
    InitVulkan();
    InitAllocator();
    InitSwapchain();
    InitDrawImages();
    InitFrameData();
    InitImmediateBuffer();
    InitDefaultImages();
    InitDefaultSamplers();

    LOG_INFO(DISPLAY, "GFX context initialized");
}

void GfxContext::InitWindow() {
    LOG_DEBUG(DISPLAY, "Initializing window");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    constexpr auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window.pWindow             = SDL_CreateWindow(
            "JTX",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            static_cast<int>(window.extent.width),
            static_cast<int>(window.extent.height),
            windowFlags);

    int w, h;
    SDL_Vulkan_GetDrawableSize(window.pWindow, &w, &h);
    window.extent.width  = w;
    window.extent.height = h;

    LOG_DEBUG(DISPLAY, "Window Initialized");
}

void GfxContext::InitVulkan() {
    LOG_DEBUG(DISPLAY, "Initializing vulkan");

    // Volk
    const auto volkResult = volkInitialize();
    if (volkResult != VK_SUCCESS) {
        LOG_FATAL(DISPLAY, "Failed to Initialize volk");
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
        LOG_FATAL(DISPLAY, "Failed to Initialize vkb instance");
    }
    const vkb::Instance vkbInstance = vkbInstanceResult.value();
    ctx.instance                    = vkbInstance.instance;
    ctx.debugMessenger              = vkbInstance.debug_messenger;
    volkLoadInstance(ctx.instance);

    // SDL surface
    SDL_Vulkan_CreateSurface(window.pWindow, ctx, &ctx.surface);

    // Vulkan physical device
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing  = true;
    features12.scalarBlockLayout   = true;

    vkb::PhysicalDeviceSelector pdSelector{vkbInstance};
    auto vkbPdResult = pdSelector
                               .set_minimum_version(1, 2)
                               .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                               .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                               .add_required_extension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME)
                               .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                               .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                               .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
                               .set_required_features_12(features12)
                               .set_surface(ctx)
                               .select();
    if (!vkbPdResult) {
        bRayTracingSupported = false;
        pdSelector = vkb::PhysicalDeviceSelector{vkbInstance};
        vkbPdResult = pdSelector
                              .set_minimum_version(1, 2)
                              .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                              .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                              .add_required_extension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME)
                              .set_surface(ctx)
                              .select();
        if (!vkbPdResult) {
            LOG_FATAL(DISPLAY, "Failed to select physical device with required features and extensions");
        }

        LOG_INFO(VULKAN, "Selected device does not support hardware ray tracing");
    } else {
        LOG_INFO(VULKAN, "Selected device supports hardware ray tracing");
    }

    vkb::PhysicalDevice &vkbPd = vkbPdResult.value();
    ctx.physicalDevice               = vkbPd.physical_device;

    // Vulkan device
    vkb::DeviceBuilder deviceBuilder{vkbPd};

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{};
    dynamicRendering.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRendering.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2{};
    synchronization2.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2.synchronization2 = VK_TRUE;
    synchronization2.pNext            = &dynamicRendering;

    // Check if ray tracing is supported
    auto availableExtensions = vkbPd.get_available_extensions();

    if (bRayTracingSupported) {
        LOG_INFO(VULKAN, "Enabling ray tracing features");
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructures{};
        accelerationStructures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructures.accelerationStructure = VK_TRUE;
        accelerationStructures.pNext                 = &synchronization2;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{};
        rayTracingPipeline.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingPipeline.rayTracingPipeline = VK_TRUE;
        rayTracingPipeline.pNext              = &accelerationStructures;

        deviceBuilder.add_pNext(&rayTracingPipeline);
    } else {
        deviceBuilder.add_pNext(&synchronization2);
    }

    vkb::Device vkbDevice = deviceBuilder.build().value();
    ctx.device            = vkbDevice;

    volkLoadDevice(ctx.device);

    // Query RT properties if ray tracing is supported
    if (bRayTracingSupported) {
        VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &rtProperties;
        vkGetPhysicalDeviceProperties2(ctx.physicalDevice, &props);

        LOG_DEBUG(VULKAN, "Ray tracing properties:");
        LOG_DEBUG(VULKAN, "  Shader group handle size: {}", rtProperties.shaderGroupHandleSize);
        LOG_DEBUG(VULKAN, "  Max ray recursions: {}", rtProperties.maxRayRecursionDepth);
        LOG_DEBUG(VULKAN, "  Max shader group stride: {}", rtProperties.maxShaderGroupStride);
        LOG_DEBUG(VULKAN, "  Shader group base alignment: {}", rtProperties.shaderGroupBaseAlignment);
        LOG_DEBUG(VULKAN, "  Shader group handle capture replay size: {}", rtProperties.shaderGroupHandleCaptureReplaySize);
        LOG_DEBUG(VULKAN, "  Max ray dispatch invocation count: {}", rtProperties.maxRayDispatchInvocationCount);
        LOG_DEBUG(VULKAN, "  Shader group handle alignment: {}", rtProperties.shaderGroupHandleAlignment);
        LOG_DEBUG(VULKAN, "  Max ray hit attribute size: {}", rtProperties.maxRayHitAttributeSize);
    }

    // Graphics queue
    graphicsQueue.queue  = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueue.family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    LOG_DEBUG(DISPLAY, "Vulkan Initialized");
}

void GfxContext::InitAllocator() {
    LOG_DEBUG(DISPLAY, "Initializing allocator");

    VmaVulkanFunctions vkFunctions{};
    vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vkFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice   = ctx;
    allocatorInfo.device           = ctx;
    allocatorInfo.instance         = ctx;
    allocatorInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.pVulkanFunctions = &vkFunctions;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    LOG_DEBUG(DISPLAY, "Allocator initialized");
}

void GfxContext::InitSwapchain() {
    LOG_DEBUG(DISPLAY, "Initializing swapchain");
    swapchain.Init(ctx, window.extent.width, window.extent.height);
    LOG_DEBUG(DISPLAY, "Swapchain Initialized");
}

void GfxContext::InitDrawImages() {
    LOG_DEBUG(DISPLAY, "Initializing draw image");
    // Draw image
    VkExtent3D drawExtent{};
    drawExtent.width  = window.extent.width;
    drawExtent.height = window.extent.height;
    drawExtent.depth  = 1;

    drawImage.image.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.image.imageExtent = drawExtent;

    VkImageUsageFlags drawImageUsages = {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;    // Copy from image
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;    // Copy to image
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// Graphics pipeline

    const VkImageCreateInfo drawImageInfo = jvk::init::Image(drawImage.image.imageFormat, drawImageUsages, drawImage.image.imageExtent);

    VmaAllocationCreateInfo drawImageAllocInfo{};
    drawImageAllocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(allocator, &drawImageInfo, &drawImageAllocInfo, &drawImage.image.image, &drawImage.image.allocation, nullptr);

    const VkImageViewCreateInfo imageViewInfo = jvk::init::ImageView(drawImage.image.imageFormat, drawImage.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(ctx, &imageViewInfo, nullptr, &drawImage.image.imageView));

    // Depth/stencil image
    jvk::GetSupportedDepthStencilFormat(ctx, &drawImage.depthStencilImage.imageFormat);
    drawImage.depthStencilImage.imageExtent = drawExtent;

    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    const VkImageCreateInfo depthImageInfo = jvk::init::Image(drawImage.depthStencilImage.imageFormat, depthImageUsages, drawExtent);
    vmaCreateImage(allocator, &depthImageInfo, &drawImageAllocInfo, &drawImage.depthStencilImage.image, &drawImage.depthStencilImage.allocation, nullptr);

    const VkImageViewCreateInfo depthImageViewInfo = jvk::init::ImageView(drawImage.depthStencilImage.imageFormat, drawImage.depthStencilImage.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    CHECK_VK(vkCreateImageView(ctx, &depthImageViewInfo, nullptr, &drawImage.depthStencilImage.imageView));
    LOG_DEBUG(DISPLAY, "Draw image Initialized");
}

void GfxContext::InitFrameData() {
    LOG_DEBUG(DISPLAY, "Initializing frame data");
    constexpr VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    for (auto &frame: frameData) {
        // Command pools
        CHECK_VK(frame.cmdPool.Init(ctx, graphicsQueue.family, flags));
        CHECK_VK(frame.cmdPool.AllocateCommandBuffer(&frame.cmdBuffer));

        // Synchronization
        CHECK_VK(frame.drawFence.Init(ctx, VK_FENCE_CREATE_SIGNALED_BIT));
        CHECK_VK(frame.swapchainSemaphore.Init(ctx));
        CHECK_VK(frame.drawSemaphore.Init(ctx));
    }
    LOG_DEBUG(DISPLAY, "Frame data Initialized");
}

void GfxContext::InitImmediateBuffer() {
    LOG_DEBUG(DISPLAY, "Initializing immediate buffer");
    CHECK_VK(imBuffer.Init(ctx, graphicsQueue.family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
    LOG_DEBUG(DISPLAY, "Immediate buffer Initialized");
}

void GfxContext::InitDefaultImages() {
    LOG_DEBUG(DISPLAY, "Initializing default images");

    const uint32_t white = jtx::packUnorm4x8({1.0f, 1.0f, 1.0f, 1.0f});
    defaultImages.white  = CreateImage(&white, VkExtent3D{1, 1, 1}, 4, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    const uint32_t black = jtx::packUnorm4x8({0.0f, 0.0f, 0.0f, 1.0f});
    defaultImages.black  = CreateImage(&black, VkExtent3D{1, 1, 1}, 4, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    // Error checkerboard
    const uint32_t magenta = jtx::packUnorm4x8({1.0f, 0.0f, 1.0f, 1.0f});
    uint32_t pixels[16 * 16];
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    defaultImages.checkerboard = CreateImage(pixels, VkExtent3D{16, 16, 1}, 4, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    LOG_DEBUG(DISPLAY, "Default images initialized");
}

void GfxContext::InitDefaultSamplers() {
    LOG_DEBUG(DISPLAY, "Initializing default samplers");

    CHECK_VK(defaultSamplers.linear.Init(ctx, VK_FILTER_LINEAR, VK_FILTER_LINEAR));
    CHECK_VK(defaultSamplers.nearest.Init(ctx, VK_FILTER_NEAREST, VK_FILTER_NEAREST));

    LOG_DEBUG(DISPLAY, "Default samplers initialized");
}

#pragma endregion

#pragma region Destruction

void GfxContext::Destroy() {
    DestroyDefaultSamplers();
    DestroyDefaultImages();
    DestroyImmediateBuffer();
    DestroyFrameData();
    DestroyDrawImages();
    DestroySwapchain();
    DestroyAllocator();
    DestroyVulkan();
    DestroyWindow();
}

void GfxContext::DestroyWindow() const {
    LOG_DEBUG(DISPLAY, "Destroying window");

    SDL_DestroyWindow(window.pWindow);

    LOG_DEBUG(DISPLAY, "Window Destroyed");
}

void GfxContext::DestroyVulkan() const {
    LOG_DEBUG(DISPLAY, "Destroying Vulkan context");

    ctx.Destroy();

    LOG_DEBUG(DISPLAY, "Vulkan context Destroyed");
}

void GfxContext::DestroyAllocator() const {
    LOG_DEBUG(DISPLAY, "Destroying allocators");

    vmaDestroyAllocator(allocator);

    LOG_DEBUG(DISPLAY, "Allocators Destroyed");
}

void GfxContext::DestroySwapchain() const {
    LOG_DEBUG(DISPLAY, "Destroying swapchain");

    swapchain.Destroy(ctx);

    LOG_DEBUG(DISPLAY, "Swapchain Destroyed");
}

void GfxContext::DestroyDrawImages() const {
    LOG_DEBUG(DISPLAY, "Destroying draw images");

    drawImage.image.Destroy(ctx, allocator);
    drawImage.depthStencilImage.Destroy(ctx, allocator);

    LOG_DEBUG(DISPLAY, "Draw images destroyed");
}

void GfxContext::DestroyFrameData() {
    LOG_DEBUG(DISPLAY, "Destroying frame data");

    for (auto &frame: frameData) {
        frame.drawSemaphore.Destroy();
        frame.swapchainSemaphore.Destroy();
        frame.drawFence.Destroy();
        frame.cmdPool.Destroy();
    }

    LOG_DEBUG(DISPLAY, "Frame data destroyed");
}

void GfxContext::DestroyImmediateBuffer() const {
    LOG_DEBUG(DISPLAY, "Destroying immediate buffer");

    imBuffer.Destroy();

    LOG_DEBUG(DISPLAY, "Immediate buffer destroyed");
}

void GfxContext::DestroyDefaultImages() const {
    LOG_DEBUG(DISPLAY, "Destroying default images");

    DestroyImage(defaultImages.white);
    DestroyImage(defaultImages.black);
    DestroyImage(defaultImages.checkerboard);

    LOG_DEBUG(DISPLAY, "Default images destroyed");
}

void GfxContext::DestroyDefaultSamplers() const {
    LOG_DEBUG(DISPLAY, "Destroying default samplers");

    defaultSamplers.nearest.Destroy();
    defaultSamplers.linear.Destroy();

    LOG_DEBUG(DISPLAY, "Default samplers destroyed");
}

#pragma endregion

#pragma region GFX resources

jvk::Image GfxContext::CreateImage(const VkExtent3D extent, const VkFormat format, const VkImageUsageFlags usage, const bool bMipmapped, const VkSampleCountFlagBits sampleCount) const {
    jvk::Image image;
    image.imageFormat         = format;
    image.imageExtent         = extent;
    VkImageCreateInfo imgInfo = jvk::init::Image(format, usage, extent, sampleCount);
    if (bMipmapped) {
        imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height))) + 1);
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_VK(vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (jvk::FormatHasDepth(image.imageFormat)) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format > VK_FORMAT_D16_UNORM_S8_UINT) {
            aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    }

    VkImageViewCreateInfo viewInfo       = jvk::init::ImageView(format, image.image, aspectFlags);
    viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;
    CHECK_VK(vkCreateImageView(ctx, &viewInfo, nullptr, &image.imageView));

    return image;
}

jvk::Image GfxContext::CreateImage(const void *pData, const VkExtent3D extent, const size_t nChannels, const VkFormat format, const VkImageUsageFlags usage, const bool bMipmapped, VkSampleCountFlagBits sampleCount) const {
    // Staging buffer, we will always assume data has 4 channels
    const size_t dataSize           = extent.width * extent.height * extent.depth * nChannels;
    const jvk::Buffer stagingBuffer = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    memcpy(stagingBuffer.info.pMappedData, pData, dataSize);

    VkImageUsageFlags imgUsages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage;
    if (bMipmapped) imgUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    const jvk::Image image = CreateImage(extent, format, imgUsages, bMipmapped);
    imBuffer.SubmitAndWait(graphicsQueue, [&](const VkCommandBuffer cmd) {
        jvk::TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset      = 0;
        copyRegion.bufferRowLength   = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = extent;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        if (bMipmapped) {
            jvk::GenerateMipmaps(cmd, image.image, {image.imageExtent.width, image.imageExtent.height});
        }

        jvk::TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    DestroyBuffer(stagingBuffer);
    return image;
}

void GfxContext::DestroyImage(const jvk::Image &image) const {
    image.Destroy(ctx, allocator);
}

jvk::Buffer GfxContext::CreateBuffer(const size_t allocSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memUsage, const VmaAllocationCreateFlags memFlags, const VkMemoryPropertyFlags memPropFlags) const {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.size  = allocSize;
    info.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage         = memUsage;
    allocInfo.flags         = memFlags;
    allocInfo.requiredFlags = memPropFlags;

    jvk::Buffer buffer{};
    CHECK_VK(vmaCreateBuffer(allocator, &info, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    return buffer;
}

void GfxContext::DestroyBuffer(const jvk::Buffer &buffer) const {
    buffer.Destroy(allocator);
}

#pragma endregion

#pragma region Frame management

void GfxContext::ResizeSwapchain() {
    if (m_bSwapchainOutOfDate) {
        LOG_DEBUG(DISPLAY, "Resizing swapchain");
        vkDeviceWaitIdle(ctx);
        swapchain.Destroy(ctx);

        int w, h;
        SDL_Vulkan_GetDrawableSize(window.pWindow, &w, &h);
        window.extent.width  = w;
        window.extent.height = h;

        swapchain.Init(ctx, window.extent.width, window.extent.height);
        m_bSwapchainOutOfDate = false;

        LOG_DEBUG(DISPLAY, "Swapchain resized");
    }
}

std::optional<RenderContext> GfxContext::StartFrame() {
    const uint32_t frameIndex = GetCurrentFrameIndex();
    const auto &frame         = frameData[frameIndex];
    CHECK_VK(frame.drawFence.Wait());
    CHECK_VK(frame.drawFence.Reset());

    uint32_t swapchainIndex;
    if (const VkResult e = swapchain.AcquireNextImage(ctx, frame.swapchainSemaphore, &swapchainIndex); e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR) {
        m_bSwapchainOutOfDate = true;
        return {};
    }

    CHECK_VK(frame.cmdBuffer.Reset());

    drawImage.extent.width  = static_cast<uint32_t>(static_cast<float>(std::min(swapchain.extent.width, drawImage.image.imageExtent.width)) * drawImage.renderScale);
    drawImage.extent.height = static_cast<uint32_t>(static_cast<float>(std::min(swapchain.extent.height, drawImage.image.imageExtent.height)) * drawImage.renderScale);

    CHECK_VK(frame.cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));

    return RenderContext{
            .cmd            = frame.cmdBuffer,
            .swapchainIndex = swapchainIndex,
            .frameIndex     = frameIndex,
            .swapchain      = {
                         .image  = swapchain.images[swapchainIndex],
                         .view   = swapchain.views[swapchainIndex],
                         .extent = swapchain.extent},
            .drawImage         = drawImage.image,
            .depthStencilImage = drawImage.depthStencilImage,
            .layout            = {
                               .swapchain         = VK_IMAGE_LAYOUT_UNDEFINED,
                               .drawImage         = VK_IMAGE_LAYOUT_UNDEFINED,
                               .depthStencilImage = VK_IMAGE_LAYOUT_UNDEFINED,
            }};
}

void GfxContext::EndFrame(const RenderContext &renderCtx) {
    // Transition the swapchain image to present layout
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.swapchain.image, renderCtx.layout.swapchain, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    CHECK_VK(renderCtx.cmd.End());

    // Submit the command buffer
    const auto &frame = frameData[renderCtx.frameIndex];

    const VkCommandBufferSubmitInfoKHR submitInfo = renderCtx.cmd.SubmitInfo();
    const VkSemaphoreSubmitInfoKHR waitInfo       = frame.swapchainSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    const VkSemaphoreSubmitInfoKHR signalInfo     = frame.drawSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR);
    graphicsQueue.Submit(&submitInfo, &waitInfo, &signalInfo, frame.drawFence);

    // Present the swapchain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain.swapchain;
    presentInfo.pWaitSemaphores    = &frame.drawSemaphore.semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &renderCtx.swapchainIndex;

    if (const VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo); presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        m_bSwapchainOutOfDate = true;
    }

    frameNumber++;
}

void GfxContext::ResolveToSwapchain(RenderContext &renderCtx, const ResolveRegion &region) const {
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.drawImage.image, renderCtx.layout.drawImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.swapchain.image, renderCtx.layout.swapchain, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    jvk::CopyImageToImage(renderCtx.cmd, renderCtx.drawImage.image, renderCtx.swapchain.image, region.src, region.dst);

    renderCtx.layout.drawImage = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    renderCtx.layout.swapchain = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

#pragma endregion

}// namespace jtx
