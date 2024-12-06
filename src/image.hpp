#pragma once

#include "color.hpp"
#include "rt.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

constexpr Float MIN_INTENSITY = 0;
constexpr Float MAX_INTENSITY = 0.999;

inline Float linearToGamma(Float x) {
    return x;
//    if (x > 0) return jtx::sqrt(x);
//    return 0;
}

static Float clampIntensity(Float i) {
    return jtx::clamp(i, MIN_INTENSITY, MAX_INTENSITY);
}

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

class RGBImage {
public:
    int w_,h_;

    RGBImage() : w_(1920), h_(1080){};
    RGBImage(const int w, const int h) : w_(w), h_(h) {
        buffer.resize(w*h);
    }

    void resize(const int w, const int h) {
        w_ = w;
        h_ = h;
        buffer.resize(w * h);
    }

    void writePixel(const Color &color, int r, int c) {
        int i = r * w_ + c;
        buffer[i].R = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.r)));
        buffer[i].G = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.g)));
        buffer[i].B = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.b)));
    }

    void save(const char *path) const {
        stbi_write_png(path, w_, h_, 3, buffer.data(), w_ * 3);
    }

    RGB *data() {
        return buffer.data();
    }

private:
    std::vector<RGB> buffer;
};