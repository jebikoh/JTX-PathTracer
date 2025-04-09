#include "image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "fmt/ostream.h"
#include "logger.hpp"
#include "tinyexr.h"

void RGB8Image::save(const char *path) const {
    std::vector<unsigned char> flipped_buffer(w_ * h_ * 3);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            const int src_index                 = (y * w_ + x) * 3;
            const int dst_index                 = ((h_ - 1 - y) * w_ + x) * 3;
            flipped_buffer[dst_index]     = buffer[y * w_ + x].R;
            flipped_buffer[dst_index + 1] = buffer[y * w_ + x].G;
            flipped_buffer[dst_index + 2] = buffer[y * w_ + x].B;
        }
    }

    stbi_write_png(path, w_, h_, 3, flipped_buffer.data(), w_ * 3);
}

TextureImage &TextureImage::operator=(TextureImage &&other) noexcept {
    if (this != &other) {
        if (data_) {
            if (format_ != ImageFormat::STBI) {
                free(data_);
            } else {
                stbi_image_free(data_);
            }
        }

        data_ = other.data_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        format_ = other.format_;

        other.data_ = nullptr;
        other.width_ = other.height_ = other.channels_ = 0;
    }
    return *this;
}

TextureImage::~TextureImage() {
    if (data_) {
        if (format_ != ImageFormat::STBI) {
            free(data_);
        } else {
            stbi_image_free(data_);
        }
    }
}

bool TextureImage::load(const char *path) {
    name_                 = std::string(path);
    const std::string ext = name_.substr(name_.find_last_of(".") + 1);

    if (ext == "exr" || ext == "EXR") {
        // Use TinyEXR for EXR files
        format_ = ImageFormat::EXR;
        return loadEXR(path);
    } else {
        // Use stb_image for other formats
        format_ = ImageFormat::STBI;
        data_  = stbi_loadf(path, &width_, &height_, &channels_, 0);
        return data_ != nullptr;
    }
}

bool TextureImage::load(const unsigned char *buffer, const size_t bufferSize, const ImageFormat format) {
    if (!buffer || bufferSize == 0) {
        LOG_ERROR("Invalid texture buffer provided");
        return false;
    }

    if (format == ImageFormat::EXR) {
        format_ = ImageFormat::EXR;
        const char *err;
        const int ret = LoadEXRFromMemory(&data_, &width_, &height_, buffer, bufferSize, &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) {
                LOG_ERROR("Failed to load EXR from memory: {}", err);
                FreeEXRErrorMessage(err);
            return false;
            }
        }
        channels_ = 4;
        name_ = "mem_exr";
        return true;
    } else {
        format_ = ImageFormat::STBI;
        data_ = stbi_loadf_from_memory(buffer, static_cast<int>(bufferSize), &width_, &height_, &channels_, 0);
        if (!data_) {
            LOG_ERROR("Failed to load texture from memory");
            return false;
        }

        name_ = "mem_stbi";
        return true;
    }
}

bool TextureImage::loadEXR(const char *path) {
    const char *err = nullptr;
    const int ret = LoadEXR(&data_, &width_, &height_, path, &err);

    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            LOG_ERROR("Failed to load EXR from {}", err);
            FreeEXRErrorMessage(err);
        }
        return false;
    }

    channels_ = 4;
    return true;
}