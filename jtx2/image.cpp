#include "image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
#include "ui/jvk/init.hpp"

#include <filesystem>

jtx::Image8u::Image8u(const uint8_t *buffer, int w, int h, int c)
    : width(w),
      height(h),
      channels(c) {
    m_data.resize(w * h * c);
    memcpy(m_data.data(), buffer, w * h * c);
}

void jtx::Image8u::destroy() {
    width    = 0;
    height   = 0;
    channels = 0;
    m_data.resize(0);
}

jtx::Image8u jtx::Image8u::loadImage(const std::filesystem::path &path) {
    LOG_INFO("Loading 8-bit texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS_8BIT.contains(fileExt)) {
        LOG_ERROR("Unsupported image format: {}", fileExt);
        return {};
    }

    Image8u image;
    const uint8_t *data = stbi_load(path.c_str(), &image.width, &image.height, &image.channels, 0);
    if (!data) {
        LOG_ERROR("Failed to load image or image was empty: {}", path.string());
        return {};
    }

    image.m_data.resize(image.width * image.height * image.channels);
    memcpy(image.m_data.data(), data, image.width * image.height * image.channels);
    stbi_image_free((void *) data);

    LOG_INFO("Loaded 8-bit texture: {}", path.string());
    return image;
}

jtx::Image8u jtx::Image8u::loadImage(const uint8_t *buffer, const size_t size) {
    LOG_INFO("Loading 8-bit texture from memory");

    if (!buffer || size == 0) {
        LOG_ERROR("Invalid buffer provided");
        return {};
    }

    Image8u image;
    const uint8_t *data = stbi_load_from_memory(buffer, size, &image.width, &image.height, &image.channels, 0);
    if (!data) {
        LOG_ERROR("Failed to load image from memory");
        return {};
    }

    image.m_data.resize(image.width * image.height * image.channels);
    memcpy(image.m_data.data(), data, image.width * image.height * image.channels);
    stbi_image_free((void *) data);

    LOG_INFO("Loaded 8-bit texture from memory");
    return image;
}
