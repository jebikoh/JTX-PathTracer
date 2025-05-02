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

#pragma region Image8u
jtx::Image8u::Image8u(const uint8_t *buffer, const int w, const int h, const int c)
    : width(w),
      height(h),
      channels(c) {
    data = new uint8_t[w * h * c];
    memcpy(data, buffer, w * h * c);
}

jtx::Image8u::Image8u(Image8u &&other) noexcept
    : width(other.width),
      height(other.height),
      channels(other.channels),
      data(other.data) {
    other.data = nullptr;
    other.destroy();
}

jtx::Image8u &jtx::Image8u::operator=(Image8u &&other) noexcept {
    if (this != &other) {
        destroy();
        width    = other.width;
        height   = other.height;
        channels = other.channels;
        data     = other.data;

        other.data = nullptr;
        other.destroy();
    }
    return *this;
}

void jtx::Image8u::destroy() {
    width    = 0;
    height   = 0;
    channels = 0;

    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

JtxResult jtx::Image8u::load(const std::filesystem::path &path, Image8u &out, bool bApplyEOTF) {
    LOG_INFO(TEXTURE, "Loading 8-bit texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS_8BIT.contains(fileExt)) {
        LOG_ERROR(TEXTURE, "Unsupported image format: {}", fileExt);
        return JTX_ERROR_INVALID_FILE_EXTENSION;
    }

    uint8_t *data = stbi_load(reinterpret_cast<const char *>(path.c_str()), &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_ERROR(TEXTURE, "Failed to load image {}: {}", path.string(), stbi_failure_reason());
        return JTX_ERROR_FILE_LOADING;
    }

    out.data = new uint8_t[out.width * out.height * out.channels];
    memcpy(out.data, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    LOG_INFO(TEXTURE, "Loaded 8-bit texture: {}", path.string());
    return JTX_SUCCESS;
}

JtxResult jtx::Image8u::load(const uint8_t *buffer, const size_t size, Image8u &out, bool bApplyEOTF) {
    LOG_INFO(TEXTURE, "Loading 8-bit texture from memory");

    if (!buffer || size == 0) {
        LOG_ERROR(TEXTURE, "Invalid buffer provided");
        return JTX_ERROR_INVALID_DATA;
    }

    uint8_t *data = stbi_load_from_memory(buffer, size, &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_ERROR(TEXTURE, "Failed to load image from memory: {}", stbi_failure_reason());
        return JTX_ERROR_FILE_LOADING;
    }

    out.data = new uint8_t[out.width * out.height * out.channels];
    memcpy(out.data, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    LOG_INFO(TEXTURE, "Loaded 8-bit texture from memory");
    return JTX_SUCCESS;
}

jtx::Image8u jtx::Image8u::as32b(const uint8_t alpha) const {
    Image8u out(width, height, 4);
    if (channels == 4) {
        memcpy(out.data, data, width * height * 4);
    }

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            auto srcPixel = data + ((row * width + col) * channels);
            auto dstPixel = out.data + (row * out.width + col) * 4;
            dstPixel[0] = srcPixel[0];
            dstPixel[1] = srcPixel[1];
            dstPixel[2] = srcPixel[2];
            dstPixel[3] = alpha;
        }
    }

    return out;
}


#pragma endregion

#pragma region Image32f
jtx::Image32f::Image32f(const float *buffer, const int w, const int h, const int c)
    : width(w),
      height(h),
      channels(c) {
    data = new float[w * h * c];
    memcpy(data, buffer, w * h * c);
}

void jtx::Image32f::destroy() {
    width    = 0;
    height   = 0;
    channels = 0;
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

JtxResult jtx::Image32f::load(const std::filesystem::path &path, Image32f &out) {
    LOG_INFO(TEXTURE, "Loading 32-bit float texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS_32BIT.contains(fileExt)) {
        LOG_ERROR(TEXTURE, "Unsupported image format: {}", fileExt);
        return JTX_ERROR_INVALID_FILE_EXTENSION;
    }

    float *data = stbi_loadf(reinterpret_cast<const char *>(path.c_str()), &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_ERROR(TEXTURE, "Failed to load image or image was empty: {}", path.string());
        return JTX_ERROR_FILE_LOADING;
    }

    out.data = new float[out.width * out.height * out.channels];
    memcpy(out.data, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    LOG_INFO(TEXTURE, "Loaded 32-bit float texture: {}", path.string());
    return JTX_SUCCESS;
}

JtxResult jtx::Image32f::load(const uint8_t *buffer, const size_t size, Image32f &out) {
    LOG_INFO(TEXTURE, "Loading 32-bit float texture from memory");

    if (!buffer || size == 0) {
        LOG_ERROR(TEXTURE, "Invalid buffer provided");
        return JTX_ERROR_INVALID_DATA;
    }

    float *data = stbi_loadf_from_memory(buffer, size, &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_ERROR(TEXTURE, "Failed to load image from memory");
        return JTX_ERROR_FILE_LOADING;
    }

    out.data = new float[out.width * out.height * out.channels];
    memcpy(out.data, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    LOG_INFO(TEXTURE, "Loaded 32-bit float texture from memory");
    return JTX_SUCCESS;
}
#pragma endregion
