#pragma once
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "interval.hpp"
#include "rtx.hpp"
#include "stb_image_write.h"

using Color                   = jtx::Vec3d;
static const double RGB_SCALE = 255.999;

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

class RGBImage {
public:
    int w;
    int h;

    RGBImage() : w(100), h(50){};
    RGBImage(int width, int height) : w(width), h(height) {
        buffer.resize(width * height);
    }

    void resize(int width, int height) {
        w = width;
        h = height;
        buffer.resize(width * height);
    }

    void writePixel(const Color &color, int row, int col) {
        static const Interval intensity(0.000, 0.999);
        int i       = row * w + col;
        buffer[i].R = int(RGB_SCALE * intensity.clamp(color.x));
        buffer[i].G = int(RGB_SCALE * intensity.clamp(color.y));
        buffer[i].B = int(RGB_SCALE * intensity.clamp(color.z));
    }

    void save(const char *path) const {
        stbi_write_png(path, w, h, 3, buffer.data(), w * 3);
    }

private:
    std::vector<RGB> buffer;
};