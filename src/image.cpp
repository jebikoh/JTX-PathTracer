#include "image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

void RGB8Image::save(const char *path) const {
    std::vector<unsigned char> flipped_buffer(w_ * h_ * 3);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            int src_index                 = (y * w_ + x) * 3;
            int dst_index                 = ((h_ - 1 - y) * w_ + x) * 3;
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
            if (isExr_) {
                free(data_);
            } else {
                stbi_image_free(data_);
            }
        }

        data_ = other.data_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        isExr_ = other.isExr_;

        other.data_ = nullptr;
        other.width_ = other.height_ = other.channels_ = 0;
    }
    return *this;
}

TextureImage::~TextureImage() {
    if (data_) {
        if (isExr_) {
            free(data_);
        } else {
            stbi_image_free(data_);
        }
    }
}

bool TextureImage::load(const char *path) {
    path_                 = std::string(path);
    const std::string ext = path_.substr(path_.find_last_of(".") + 1);

    if (ext == "exr" || ext == "EXR") {
        // Use TinyEXR for EXR files
        isExr_ = true;
        return loadEXR(path);
    } else {
        // Use stb_image for other formats
        isExr_ = false;
        data_  = stbi_loadf(path, &width_, &height_, &channels_, 0);
        return data_ != nullptr;
    }
}

bool TextureImage::load(const unsigned char *buffer, const size_t bufferSize, const ImageFormat format) {
    if (!buffer || bufferSize == 0) {
        std::cerr << "Invalid buffer" << std::endl;
        return false;
    }

    if (format == ImageFormat::EXR) {
        isExr_ = true;
        const char *err;
        const int ret = LoadEXRFromMemory(&data_, &width_, &height_, buffer, bufferSize, &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) {
                std::cerr << "Failed to load EXR from memory: " << err << std::endl;
                FreeEXRErrorMessage(err);
            return false;
            }
        }
        channels_ = 4;
        path_ = "mem_exr";
        return true;
    } else {
        isExr_ = false;
        data_ = stbi_loadf_from_memory(buffer, static_cast<int>(bufferSize), &width_, &height_, &channels_, 0);
        if (!data_) {
            std::cerr << "Failed to load image from memory" << std::endl;
            return false;
        }

        path_ = "mem_stbi";
        return true;
    }
}

bool TextureImage::loadEXR(const char *path) {
    const char *err = nullptr;
    const int ret = LoadEXR(&data_, &width_, &height_, path, &err);

    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "Failed to load EXR file: " << err << std::endl;
            FreeEXRErrorMessage(err);
        }
        return false;
    }

    channels_ = 4;
    return true;
}