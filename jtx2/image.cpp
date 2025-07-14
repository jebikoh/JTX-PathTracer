#include <image.hpp>
#include <jvk/init.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <filesystem>

namespace jtx {
#pragma region Image8u
Image8u::Image8u(const uint8_t *buffer, const int w, const int h, const int c)
    : width(w),
      height(h),
      channels(c) {
    pData = new uint8_t[w * h * c];
    memcpy(pData, buffer, w * h * c);
}

Image8u::Image8u(Image8u &&other) noexcept
    : width(other.width),
      height(other.height),
      channels(other.channels),
      pData(other.pData) {
    other.pData = nullptr;
    other.Destroy();
}

Image8u &Image8u::operator=(Image8u &&other) noexcept {
    if (this != &other) {
        Destroy();
        width    = other.width;
        height   = other.height;
        channels = other.channels;
        pData     = other.pData;

        other.pData = nullptr;
        other.Destroy();
    }
    return *this;
}

void Image8u::Destroy() {
    width    = 0;
    height   = 0;
    channels = 0;

    if (pData != nullptr) {
        delete[] pData;
        pData = nullptr;
    }
}

JtxResult Image8u::Load(const std::filesystem::path &path, Image8u &out, bool bApplyEOTF) {
    LOG_DEBUG(TEXTURE, "Loading 8-bit texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS_8BIT.contains(fileExt)) {
        LOG_ERROR(TEXTURE, "Unsupported image format: {}", fileExt);
        return JTX_ERROR_INVALID_FILE_EXTENSION;
    }

    uint8_t *data = stbi_load(path.string().c_str(), &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_FATAL(TEXTURE, "Failed to load image {}: {}", path.string(), stbi_failure_reason());
        return JTX_ERROR_FILE_LOADING;
    }

    out.pData = new uint8_t[out.width * out.height * out.channels];
    memcpy(out.pData, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    return JTX_SUCCESS;
}

JtxResult Image8u::Load(const uint8_t *buffer, const size_t size, Image8u &out, bool bApplyEOTF) {
    LOG_DEBUG(TEXTURE, "Loading 8-bit texture from memory");

    if (!buffer || size == 0) {
        LOG_ERROR(TEXTURE, "Invalid buffer provided");
        return JTX_ERROR_INVALID_DATA;
    }

    uint8_t *data = stbi_load_from_memory(buffer, size, &out.width, &out.height, &out.channels, 0);
    if (!data) {
        LOG_ERROR(TEXTURE, "Failed to load image from memory: {}", stbi_failure_reason());
        return JTX_ERROR_FILE_LOADING;
    }

    out.pData = new uint8_t[out.width * out.height * out.channels];
    memcpy(out.pData, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    return JTX_SUCCESS;
}

JtxResult Image8u::Save(const std::filesystem::path &path, const bool bFlip) const {
    if (bFlip) {
        std::vector<uint8_t> flipped(width * height * channels);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int srcIndex = (y * width + x) * channels;
                const int dstIndex = ((height - 1 - y) * width + x) * channels;
                for (int c = 0; c < channels; ++c) {
                    flipped[dstIndex + c] = pData[srcIndex + c];
                }
            }
        }
        if (stbi_write_png(path.string().c_str(), width, height, channels, flipped.data(), width * channels)) {
            return JTX_SUCCESS;
        }
    } else {
        if (stbi_write_png(path.string().c_str(), width, height, channels, pData, width * channels)) {
            return JTX_SUCCESS;
        }
    }

    LOG_ERROR(TEXTURE, "Failed to save image to file: {}", stbi_failure_reason());
    return JTX_ERROR_FILE_WRITE;
}

Image8u Image8u::As32b(const uint8_t alpha) const {
    Image8u out(width, height, 4);
    if (channels == 4) {
        memcpy(out.pData, pData, width * height * 4);
    }

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            auto srcPixel = pData + ((row * width + col) * channels);
            auto dstPixel = out.pData + (row * out.width + col) * 4;
            dstPixel[0]   = srcPixel[0];
            dstPixel[1]   = srcPixel[1];
            dstPixel[2]   = srcPixel[2];
            dstPixel[3]   = alpha;
        }
    }

    return out;
}

vec3 Image8u::SampleRGB(const vec2 &tx) const {
    int wx = FloatToUNORM8(tx.x) % width;
    if (wx < 0) wx += width;

    int wy = FloatToUNORM8(tx.y) % height;
    if (wy < 0) wy += height;

    const auto *pixel = pData + (wx * width + wy) * channels;
    return vec3(UNORM8ToFloat(pixel[0]), UNORM8ToFloat(pixel[1]), UNORM8ToFloat(pixel[2]));
}


#pragma endregion

#pragma region Image32f
Image32f::Image32f(const float *buffer, const int w, const int h, const int c)
    : width(w),
      height(h),
      channels(c) {
    pData = new float[w * h * c];
    memcpy(pData, buffer, w * h * c);
}

Image32f::Image32f(Image32f &other)
    : width(other.width),
      height(other.height),
      channels(other.channels),
      pData(other.pData) {
    other.pData = nullptr;
    other.Destroy();
}
Image32f &Image32f::operator=(Image32f &other) {
    if (this != &other) {
        Destroy();
        width    = other.width;
        height   = other.height;
        channels = other.channels;
        pData     = other.pData;

        other.pData = nullptr;
        other.Destroy();
    }
    return *this;
}

void Image32f::Destroy() {
    width    = 0;
    height   = 0;
    channels = 0;
    if (pData != nullptr) {
        delete[] pData;
        pData = nullptr;
    }
}

JtxResult Image32f::Load(const std::filesystem::path &path, Image32f &out) {
    LOG_INFO(TEXTURE, "Loading 32-bit float texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS_32BIT.contains(fileExt)) {
        LOG_ERROR(TEXTURE, "Unsupported image format: {}", fileExt);
        return JTX_ERROR_INVALID_FILE_EXTENSION;
    }

    if (fileExt == ".exr") {
        LOG_DEBUG(TEXTURE, "Loading EXR");
        const char *err = nullptr;
        const int ret = LoadEXR(&out.pData, &out.width, &out.height, path.string().c_str(), &err);

        if (ret != TINYEXR_SUCCESS) {
            if (err) {
                LOG_ERROR(TEXTURE, "Failed to load EXR: {}", err);
                FreeEXRErrorMessage(err);
                return JTX_ERROR_FILE_LOADING;
            }
            return JTX_FAILURE;
        }

        out.channels = 4;
        return JTX_SUCCESS;
    } else {
        float *data = stbi_loadf(reinterpret_cast<const char *>(path.c_str()), &out.width, &out.height, &out.channels, 0);
        if (!data) {
            LOG_ERROR(TEXTURE, "Failed to load image or image was empty: {}", path.string());
            return JTX_ERROR_FILE_LOADING;
        }

        out.pData = new float[out.width * out.height * out.channels];
        memcpy(out.pData, data, out.width * out.height * out.channels);
        stbi_image_free(data);
    }

    LOG_INFO(TEXTURE, "Loaded 32-bit float texture: {}", path.string());
    return JTX_SUCCESS;
}

JtxResult Image32f::Load(const uint8_t *buffer, const size_t size, Image32f &out) {
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

    out.pData = new float[out.width * out.height * out.channels];
    memcpy(out.pData, data, out.width * out.height * out.channels);
    stbi_image_free(data);

    LOG_INFO(TEXTURE, "Loaded 32-bit float texture from memory");
    return JTX_SUCCESS;
}

vec3 Image32f::SampleRGB(const vec2 &tx) const {
    auto wx = static_cast<int32_t>(tx.x * width);
    auto wy = static_cast<int32_t>(tx.y * height);

    wx = std::clamp(wx, 0, width - 1);
    wy = std::clamp(wy, 0, height - 1);

    const auto *pixel = pData + (wy * width + wx) * channels;
    return vec3(pixel);
}

float CalculateMSE(const Image8u &img, const Image8u &ref) {
    if (ref.width != img.width || ref.height != img.height || ref.channels != img.channels) {
        LOG_ERROR(TEXTURE, "Images must have the same dimensions and channels for MSE calculation");
        return -1.0f;
    }

    float error = 0.0f;
    for (int i = 0; i < ref.width * ref.height * ref.channels; ++i) {
        const float diff = static_cast<float>(ref.pData[i]) - static_cast<float>(img.pData[i]);
        error += diff * diff;
    }

    return error / static_cast<float>(ref.width * ref.height * ref.channels);
}
#pragma endregion
}