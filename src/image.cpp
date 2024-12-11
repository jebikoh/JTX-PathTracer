#include "image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void RGBImage::save(const char *path) const {
    stbi_write_png(path, w_, h_, 3, buffer.data(), w_ * 3);
}