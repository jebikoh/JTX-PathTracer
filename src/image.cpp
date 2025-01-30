#include "image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void RGBImage::save(const char *path) const {
    std::vector<unsigned char> flipped_buffer(w_ * h_ * 3);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            int src_index = (y * w_ + x) * 3;
            int dst_index = ((h_ - 1 - y) * w_ + x) * 3;
            flipped_buffer[dst_index] = buffer[y * w_ + x].R;
            flipped_buffer[dst_index + 1] = buffer[y * w_ + x].G;
            flipped_buffer[dst_index + 2] = buffer[y * w_ + x].B;
        }
    }

    stbi_write_png(path, w_, h_, 3, flipped_buffer.data(), w_ * 3);
}