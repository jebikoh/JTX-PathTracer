#define VOLK_IMPLEMENTATION
#include <editor/gfx_context.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <jvk/util.hpp>
#include <util/profiling.hpp>

#ifdef NDEBUG
constexpr bool JTX_USE_VALIDATION_LAYERS = false;
#else
constexpr bool JTX_USE_VALIDATION_LAYERS = true;
#endif

namespace jtx {

#pragma region Initialization

void GfxContext::Init() {
    TPROFILE_SCOPE();

    LOG_INFO(GFX, "Initializing GFX context");

    InitWindow();
    InitVulkan();
    InitAllocator();
    InitSwapchain();
    InitRenderTargets();
    InitFrameData();
    InitImmediateBuffer();
    InitDefaultImages();
    InitDefaultSamplers();

    LOG_INFO(GFX, "GFX context initialized");
}

void GfxContext::InitWindow() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing window");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    constexpr auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    window.pWindow             = SDL_CreateWindow(
            "JTX",
            1700,
            800,
            windowFlags);
    window.id = SDL_GetWindowID(window.pWindow);

    int w, h;
    SDL_GetWindowSizeInPixels(window.pWindow, &w, &h);
    window.extent.width  = w;
    window.extent.height = h;

    LOG_DEBUG(GFX, "Window Initialized");
}

void GfxContext::InitVulkan() {
    TPROFILE_SCOPE();

    LOG_DEBUG(GFX, "Initializing vulkan");

    // Volk
    const auto volkResult = volkInitialize();
    if (volkResult != VK_SUCCESS) {
        LOG_FATAL(GFX, "Failed to Initialize volk");
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
        LOG_FATAL(GFX, "Failed to Initialize vkb instance");
    }
    const vkb::Instance vkbInstance = vkbInstanceResult.value();
    ctx.instance                    = vkbInstance.instance;
    ctx.debugMessenger              = vkbInstance.debug_messenger;
    volkLoadInstance(ctx.instance);

    // SDL surface
    SDL_Vulkan_CreateSurface(window.pWindow, ctx, nullptr, &ctx.surface);

    // The slang shaders for metal need these
    VkPhysicalDeviceVulkan11Features features11{};
    features11.sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.variablePointers              = VK_TRUE;
    features11.variablePointersStorageBuffer = VK_TRUE;

    // Vulkan physical device
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress                           = VK_TRUE;
    features12.descriptorIndexing                            = VK_TRUE;
    features12.descriptorBindingPartiallyBound               = VK_TRUE;
    features12.shaderStorageBufferArrayNonUniformIndexing    = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
    features12.shaderStorageImageArrayNonUniformIndexing     = VK_TRUE;
    features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
    features12.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
    features12.runtimeDescriptorArray                        = VK_TRUE;
    features12.scalarBlockLayout                             = VK_TRUE;

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
                               .set_required_features_11(features11)
                               .set_surface(ctx)
                               .select();
    if (!vkbPdResult) {
        bRayTracingSupported = false;
        pdSelector           = vkb::PhysicalDeviceSelector{vkbInstance};
        vkbPdResult          = pdSelector
                              .set_minimum_version(1, 2)
                              .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                              .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                              .add_required_extension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME)
                              .set_required_features_12(features12)
                              .set_required_features_11(features11)
                              .set_surface(ctx)
                              .select();
        if (!vkbPdResult) {
            LOG_FATAL(GFX, "Failed to select physical device with required features and extensions");
        }

        LOG_INFO(GFX, "Selected device does not support hardware ray tracing");
    } else {
        LOG_INFO(GFX, "Selected device supports hardware ray tracing");
    }

    vkb::PhysicalDevice &vkbPd = vkbPdResult.value();
    ctx.physicalDevice         = vkbPd.physical_device;

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
        LOG_INFO(GFX, "Enabling ray tracing features");
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructures{};
        accelerationStructures.sType                                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructures.accelerationStructure                                 = VK_TRUE;
        accelerationStructures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
        accelerationStructures.pNext                                                 = &synchronization2;

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
        props.pNext        = &rtProperties;
        rtProperties.pNext = &asProperties;
        vkGetPhysicalDeviceProperties2(ctx.physicalDevice, &props);

        LOG_DEBUG(GFX, "Ray tracing properties:");
        LOG_DEBUG(GFX, "  Shader group handle size: {}", rtProperties.shaderGroupHandleSize);
        LOG_DEBUG(GFX, "  Max ray recursions: {}", rtProperties.maxRayRecursionDepth);
        LOG_DEBUG(GFX, "  Max shader group stride: {}", rtProperties.maxShaderGroupStride);
        LOG_DEBUG(GFX, "  Shader group base alignment: {}", rtProperties.shaderGroupBaseAlignment);
        LOG_DEBUG(GFX, "  Shader group handle capture replay size: {}", rtProperties.shaderGroupHandleCaptureReplaySize);
        LOG_DEBUG(GFX, "  Max ray dispatch invocation count: {}", rtProperties.maxRayDispatchInvocationCount);
        LOG_DEBUG(GFX, "  Shader group handle alignment: {}", rtProperties.shaderGroupHandleAlignment);
        LOG_DEBUG(GFX, "  Max ray hit attribute size: {}", rtProperties.maxRayHitAttributeSize);
    }

    // Graphics queue
    graphicsQueue.queue  = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueue.family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    LOG_DEBUG(GFX, "Vulkan Initialized");
}

void GfxContext::InitAllocator() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing allocator");

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

    LOG_DEBUG(GFX, "Allocator initialized");
}

void GfxContext::InitSwapchain() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing swapchain");
    window.swapchain.Init(ctx, window.extent.width, window.extent.height);

    const uint32_t count = window.swapchain.GetSwapchainImageCount();
    window.semaphores.resize(count);
    for (auto &sem: window.semaphores) {
        sem.Init(ctx);
    }
    LOG_DEBUG(GFX, "Swapchain Initialized");
}

void GfxContext::InitRenderTargets() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing render targets");

    // TODO:
    // Right now, the draw images are initialized to the maximum screen size
    // Lets adjust this later to save some memory and have it resized dynamically
    SDL_DisplayID displayId = SDL_GetDisplayForWindow(window.pWindow);
    const auto dm = SDL_GetCurrentDisplayMode(displayId);
    if (dm == nullptr) {
        LOG_FATAL(GFX, "SDL error while retrieving display mode: {}", SDL_GetError());
    }

    // Draw image 16f
    VkExtent3D drawExtent{};
    drawExtent.width  = static_cast<uint32_t>(dm->w * dm->pixel_density);
    drawExtent.height = static_cast<uint32_t>(dm->h * dm->pixel_density);
    drawExtent.depth  = 1;

    targets.draw16f.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    targets.draw16f.extent = drawExtent;

    VkImageUsageFlags drawImageUsages = {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;    // Copy from image
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;    // Copy to image
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// Graphics pipeline

    VkImageCreateInfo drawImageInfo = jvk::init::Image(targets.draw16f.format, drawImageUsages, targets.draw16f.extent);

    VmaAllocationCreateInfo drawImageAllocInfo{};
    drawImageAllocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(allocator, &drawImageInfo, &drawImageAllocInfo, &targets.draw16f.image, &targets.draw16f.allocation, nullptr);

    VkImageViewCreateInfo imageViewInfo = jvk::init::ImageView(targets.draw16f.format, targets.draw16f.image, VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK_VK(vkCreateImageView(ctx, &imageViewInfo, nullptr, &targets.draw16f.view));

    // Draw image 32f
    if (bRayTracingSupported) {
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        targets.draw32f.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        targets.draw32f.extent = drawExtent;
        drawImageInfo  = jvk::init::Image(targets.draw32f.format, drawImageUsages, targets.draw32f.extent);

        vmaCreateImage(allocator, &drawImageInfo, &drawImageAllocInfo, &targets.draw32f.image, &targets.draw32f.allocation, nullptr);

        imageViewInfo = jvk::init::ImageView(targets.draw32f.format, targets.draw32f.image, VK_IMAGE_ASPECT_COLOR_BIT);
        CHECK_VK(vkCreateImageView(ctx, &imageViewInfo, nullptr, &targets.draw32f.view));
    }

    // Depth/stencil image
    jvk::GetSupportedDepthStencilFormat(ctx, &targets.depthStencil.format);
    targets.depthStencil.extent = drawExtent;

    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    const VkImageCreateInfo depthImageInfo = jvk::init::Image(targets.depthStencil.format, depthImageUsages, drawExtent);
    vmaCreateImage(allocator, &depthImageInfo, &drawImageAllocInfo, &targets.depthStencil.image, &targets.depthStencil.allocation, nullptr);

    const VkImageViewCreateInfo depthImageViewInfo = jvk::init::ImageView(targets.depthStencil.format, targets.depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    CHECK_VK(vkCreateImageView(ctx, &depthImageViewInfo, nullptr, &targets.depthStencil.view));
    LOG_DEBUG(GFX, "Render targets Initialized");
}

void GfxContext::InitFrameData() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing frame data");

    constexpr VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    for (auto &frame: frameData) {
        // Command pools
        CHECK_VK(frame.cmdPool.Init(ctx, graphicsQueue.family, flags));
        CHECK_VK(frame.cmdPool.AllocateCommandBuffer(&frame.cmdBuffer));

        // Synchronization
        CHECK_VK(frame.drawFence.Init(ctx, VK_FENCE_CREATE_SIGNALED_BIT));
        CHECK_VK(frame.imageAvailableSemaphore.Init(ctx));// Ignore this for now
    }

    LOG_DEBUG(GFX, "Frame data Initialized");
}

void GfxContext::InitImmediateBuffer() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing immediate buffer");
    CHECK_VK(imBuffer.Init(ctx, graphicsQueue.family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
    LOG_DEBUG(GFX, "Immediate buffer Initialized");
}

void GfxContext::InitDefaultImages() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing default images");

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

    LOG_DEBUG(GFX, "Default images initialized");
}

void GfxContext::InitDefaultSamplers() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Initializing default samplers");

    CHECK_VK(defaultSamplers.linear.Init(ctx, VK_FILTER_LINEAR, VK_FILTER_LINEAR));
    CHECK_VK(defaultSamplers.nearest.Init(ctx, VK_FILTER_NEAREST, VK_FILTER_NEAREST));

    LOG_DEBUG(GFX, "Default samplers initialized");
}

#pragma endregion

#pragma region Destruction

void GfxContext::Destroy() {
    TPROFILE_SCOPE();
    DestroyDefaultSamplers();
    DestroyDefaultImages();
    DestroyImmediateBuffer();
    DestroyFrameData();
    DestroyRenderTargets();
    DestroySwapchain();
    DestroyAllocator();
    DestroyVulkan();
    DestroyWindow();
}

void GfxContext::DestroyWindow() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying window");

    SDL_DestroyWindow(window.pWindow);

    LOG_DEBUG(GFX, "Window Destroyed");
}

void GfxContext::DestroyVulkan() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying Vulkan context");

    ctx.Destroy();

    LOG_DEBUG(GFX, "Vulkan context Destroyed");
}

void GfxContext::DestroyAllocator() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying allocators");

    vmaDestroyAllocator(allocator);

    LOG_DEBUG(GFX, "Allocators Destroyed");
}

void GfxContext::DestroySwapchain() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying swapchain");

    for (auto &sem: window.semaphores) {
        sem.Destroy();
    }

    window.swapchain.Destroy(ctx);

    LOG_DEBUG(GFX, "Swapchain Destroyed");
}

void GfxContext::DestroyRenderTargets() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying render targets");

    if (bRayTracingSupported) targets.draw32f.Destroy(ctx, allocator);

    targets.draw16f.Destroy(ctx, allocator);
    targets.depthStencil.Destroy(ctx, allocator);

    LOG_DEBUG(GFX, "Render targets destroyed");
}

void GfxContext::DestroyFrameData() {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying frame data");

    for (auto &frame: frameData) {
        frame.imageAvailableSemaphore.Destroy();
        frame.drawFence.Destroy();
        frame.cmdPool.Destroy();
    }

    LOG_DEBUG(GFX, "Frame data destroyed");
}

void GfxContext::DestroyImmediateBuffer() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying immediate buffer");

    imBuffer.Destroy();

    LOG_DEBUG(GFX, "Immediate buffer destroyed");
}

void GfxContext::DestroyDefaultImages() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying default images");

    DestroyImage(defaultImages.white);
    DestroyImage(defaultImages.black);
    DestroyImage(defaultImages.checkerboard);

    LOG_DEBUG(GFX, "Default images destroyed");
}

void GfxContext::DestroyDefaultSamplers() const {
    TPROFILE_SCOPE();
    LOG_DEBUG(GFX, "Destroying default samplers");

    defaultSamplers.nearest.Destroy();
    defaultSamplers.linear.Destroy();

    LOG_DEBUG(GFX, "Default samplers destroyed");
}

#pragma endregion

#pragma region GFX resources

jvk::Image GfxContext::CreateImage(const VkExtent3D extent, const VkFormat format, const VkImageUsageFlags usage, const bool bMipmapped, const VkSampleCountFlagBits sampleCount) const {
    TPROFILE_SCOPE();
    jvk::Image image;
    image.format              = format;
    image.extent              = extent;
    VkImageCreateInfo imgInfo = jvk::init::Image(format, usage, extent, sampleCount);
    if (bMipmapped) {
        imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height))) + 1);
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_VK(vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (jvk::FormatHasDepth(image.format)) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format > VK_FORMAT_D16_UNORM_S8_UINT) {
            aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    }

    VkImageViewCreateInfo viewInfo       = jvk::init::ImageView(format, image.image, aspectFlags);
    viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;
    CHECK_VK(vkCreateImageView(ctx, &viewInfo, nullptr, &image.view));

    return image;
}

jvk::Image GfxContext::CreateImage(const void *pData, const VkExtent3D extent, const size_t nChannels, const VkFormat format, const VkImageUsageFlags usage, const size_t bytesPerPixel, const bool bMipmapped, VkSampleCountFlagBits sampleCount) const {
    TPROFILE_SCOPE();
    const size_t dataSize     = extent.width * extent.height * extent.depth * nChannels * bytesPerPixel;
    jvk::Buffer stagingBuffer = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
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
            jvk::GenerateMipmaps(cmd, image.image, {image.extent.width, image.extent.height});
        }

        jvk::TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    DestroyBuffer(stagingBuffer);
    return image;
}

void GfxContext::DestroyImage(const jvk::Image &image) const {
    TPROFILE_SCOPE();
    image.Destroy(ctx, allocator);
}

jvk::Buffer GfxContext::CreateBuffer(
        const size_t allocSize,
        const VkBufferUsageFlags usage,
        const VmaMemoryUsage memUsage,
        const VmaAllocationCreateFlags memFlags,
        const VkMemoryPropertyFlags memPropFlags,
        const VkDeviceSize minAlignment) const {
    TPROFILE_SCOPE();
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
    if (minAlignment > 0) {
        CHECK_VK(vmaCreateBufferWithAlignment(allocator, &info, &allocInfo, minAlignment, &buffer.buffer, &buffer.allocation, &buffer.info));
    } else {
        CHECK_VK(vmaCreateBuffer(allocator, &info, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    }
    return buffer;
}

void GfxContext::DestroyBuffer(jvk::Buffer &buffer) const {
    TPROFILE_SCOPE();
    buffer.Destroy(allocator);
}

void GfxContext::CreateExternalWindow(const VkExtent2D extent, Window &out) const {
    TPROFILE_SCOPE();
    SDL_Init(SDL_INIT_VIDEO);
    constexpr auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    out.pWindow             = SDL_CreateWindow(
            "JTX Render",
            static_cast<int>(extent.width),
            static_cast<int>(extent.height),
            windowFlags);
    out.id = SDL_GetWindowID(out.pWindow);

    int w, h;
    SDL_GetWindowSizeInPixels(out.pWindow, &w, &h);
    out.extent.width  = w;
    out.extent.height = h;

    SDL_Vulkan_CreateSurface(out.pWindow, ctx, nullptr, &out.surface);

    out.swapchain.Init(ctx, out.surface, w, h);
    const uint32_t count = out.swapchain.GetSwapchainImageCount();
    out.semaphores.resize(count);
    for (auto &sem : out.semaphores) {
        sem.Init(ctx);
    }
}

void GfxContext::DestroyExternalWindow(Window &in) const {
    TPROFILE_SCOPE();
    vkDeviceWaitIdle(ctx);
    for (auto &sem : in.semaphores) {
        sem.Destroy();
    }
    in.swapchain.Destroy(ctx);
    vkDestroySurfaceKHR(ctx, in.surface, nullptr);
    SDL_DestroyWindow(in.pWindow);
    in = {};
}

#pragma endregion

#pragma region Frame management

void GfxContext::ResizeSwapchain() {
    TPROFILE_SCOPE();
    if (window.bSwapchainOutOfDate) {
        LOG_DEBUG(GFX, "Resizing swapchain");

        vkDeviceWaitIdle(ctx);
        DestroySwapchain();

        int w, h;
        SDL_GetWindowSizeInPixels(window.pWindow, &w, &h);
        window.extent.width  = w;
        window.extent.height = h;

        InitSwapchain();
        window.bSwapchainOutOfDate = false;

        LOG_DEBUG(GFX, "Swapchain resized");
    }
}

std::optional<RenderContext> GfxContext::StartFrame() {
    TPROFILE_SCOPE();
    const uint32_t frameIndex = GetCurrentFrameIndex();
    const auto &frame          = frameData[frameIndex];
    CHECK_VK(frame.drawFence.Wait());
    CHECK_VK(frame.drawFence.Reset());

    uint32_t swapchainIndex;
    if (const VkResult e = window.swapchain.AcquireNextImage(ctx, frame.imageAvailableSemaphore, &swapchainIndex); e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR) {
        window.bSwapchainOutOfDate = true;
        return {};
    }

    CHECK_VK(frame.cmdBuffer.Reset());

    CHECK_VK(frame.cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));

    return RenderContext{
            .cmd            = frame.cmdBuffer,
            .swapchainIndex = swapchainIndex,
            .frameIndex     = frameIndex,
            .swapchain      = {
                         .image  = window.swapchain.images[swapchainIndex],
                         .view   = window.swapchain.views[swapchainIndex],
                         .extent = window.swapchain.extent
            },
            .layout  = {
                    .swapchain    = VK_IMAGE_LAYOUT_UNDEFINED,
                    .draw16f      = VK_IMAGE_LAYOUT_UNDEFINED,
                    .draw32f      = VK_IMAGE_LAYOUT_UNDEFINED,
                    .depthStencil = VK_IMAGE_LAYOUT_UNDEFINED,
            }};
}

void GfxContext::EndFrame(const RenderContext &renderCtx) {
    TPROFILE_SCOPE();
    // Transition the swapchain image to present layout
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.swapchain.image, renderCtx.layout.swapchain, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    CHECK_VK(renderCtx.cmd.End());

    // Submit the command buffer
    const auto &frame = frameData[renderCtx.frameIndex];

    const VkCommandBufferSubmitInfoKHR submitInfo = renderCtx.cmd.SubmitInfo();
    // Acquire swapchain image -> start rendering
    const VkSemaphoreSubmitInfoKHR waitInfo = frame.imageAvailableSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    // Finish rendering -> present image
    const auto &rfSemaphore                   = window.semaphores[renderCtx.swapchainIndex];
    const VkSemaphoreSubmitInfoKHR signalInfo = rfSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR);
    graphicsQueue.Submit(&submitInfo, &waitInfo, &signalInfo, frame.drawFence);

    // Present the swapchain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &window.swapchain.swapchain;
    presentInfo.pWaitSemaphores    = &rfSemaphore.semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &renderCtx.swapchainIndex;

    if (const VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo); presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        window.bSwapchainOutOfDate = true;
    }

    frameNumber++;
}

std::optional<RenderContext> GfxContext::StartFrame(Window &exWindow) const {
    TPROFILE_SCOPE();
    const uint32_t frameIndex = GetCurrentFrameIndex();
    const auto &frame         = frameData[frameIndex];
    CHECK_VK(frame.drawFence.Wait());
    CHECK_VK(frame.drawFence.Reset());

    uint32_t swapchainIndex;
    if (const VkResult e = exWindow.swapchain.AcquireNextImage(ctx, frame.imageAvailableSemaphore, &swapchainIndex); e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR) {
        exWindow.bSwapchainOutOfDate = true;
        return {};
    }

    CHECK_VK(frame.cmdBuffer.Reset());

    CHECK_VK(frame.cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));

    return RenderContext{
            .cmd            = frame.cmdBuffer,
            .swapchainIndex = swapchainIndex,
            .frameIndex     = frameIndex,
            .swapchain      = {
                         .image  = exWindow.swapchain.images[swapchainIndex],
                         .view   = exWindow.swapchain.views[swapchainIndex],
                         .extent = exWindow.swapchain.extent},
            .layout = {
                    .swapchain    = VK_IMAGE_LAYOUT_UNDEFINED,
                    .draw16f      = VK_IMAGE_LAYOUT_UNDEFINED,
                    .draw32f      = VK_IMAGE_LAYOUT_UNDEFINED,
                    .depthStencil = VK_IMAGE_LAYOUT_UNDEFINED,
            }};
}

void GfxContext::EndFrame(const RenderContext &renderCtx, Window &exWindow) {
    TPROFILE_SCOPE();
    // Transition the swapchain image to present layout
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.swapchain.image, renderCtx.layout.swapchain, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    CHECK_VK(renderCtx.cmd.End());

    // Submit the command buffer
    const auto &frame = frameData[renderCtx.frameIndex];

    const VkCommandBufferSubmitInfoKHR submitInfo = renderCtx.cmd.SubmitInfo();
    // Acquire swapchain image -> start rendering
    const VkSemaphoreSubmitInfoKHR waitInfo = frame.imageAvailableSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    // Finish rendering -> present image
    const auto &rfSemaphore                   = exWindow.semaphores[renderCtx.swapchainIndex];
    const VkSemaphoreSubmitInfoKHR signalInfo = rfSemaphore.SubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR);
    graphicsQueue.Submit(&submitInfo, &waitInfo, &signalInfo, frame.drawFence);

    // Present the swapchain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &exWindow.swapchain.swapchain;
    presentInfo.pWaitSemaphores    = &rfSemaphore.semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &renderCtx.swapchainIndex;

    if (const VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo); presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        exWindow.bSwapchainOutOfDate = true;
    }

    frameNumber++;
}

void GfxContext::ResolveToSwapchain(RenderContext &renderCtx, const ResolveRegion &region) const {
    TPROFILE_SCOPE();
    const jvk::Image *renderTarget;
    VkImageLayout *layout;
    switch (region.target) {
        case kRenderTarget::DRAW16f:
            renderTarget = &targets.draw16f;
            layout       = &renderCtx.layout.draw16f;
            break;
        case kRenderTarget::DRAW32f:
            renderTarget = &targets.draw32f;
            layout       = &renderCtx.layout.draw32f;
            break;
        case kRenderTarget::DEPTH_STENCIL:
            renderTarget = &targets.depthStencil;
            layout       = &renderCtx.layout.depthStencil;
            break;
        default:
            LOG_FATAL(GFX, "Invalid swapchain resolve");
            return;
    }

    jvk::TransitionImageIfNeeded(renderCtx.cmd, *renderTarget, *layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    jvk::TransitionImageIfNeeded(renderCtx.cmd, renderCtx.swapchain.image, renderCtx.layout.swapchain, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    jvk::CopyImageToImage(renderCtx.cmd, *renderTarget, renderCtx.swapchain.image, region.src, region.dst);

    *layout                    = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    renderCtx.layout.swapchain = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

#pragma endregion

}// namespace jtx
