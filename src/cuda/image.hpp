#pragma once

#include "otter.hpp"
#include <thrust/fill.h>

using Color = jtx::Vec3f;
static const float RGB_SCALE = 255.999f;

static const auto WHITE = Color(1, 1, 1);
static const auto BLACK = Color(0, 0, 0);

DEV INLINE int toInt(float x) {
    return static_cast<int>(RGB_SCALE * jtx::clamp(x, 0.0f, 0.999f));
}

constexpr float MIN_INTENSITY = 0;
constexpr float MAX_INTENSITY = 0.999;

DEV INLINE float linearToGamma(float x) {
    if (x > 0) return jtx::sqrt(x);
    return 0;
}

DEV INLINE float clampIntensity(float i) {
    return jtx::clamp(i, MIN_INTENSITY, MAX_INTENSITY);
}

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

// RGB buffer for image
class RGBImage {
public:
    HOSTDEV RGBImage() : w_(1920), h_(1080) {
        CHECK_CUDA(cudaMallocManaged((void **)&buffer_, w * h * sizeof(RBG)));
    };

    HOSTDEV RGBImage(const int w, const int h) : w_(w), h_(h) {
        CHECK_CUDA(cudaMallocManaged((void **)&buffer_, w * h * sizeof(RBG)));
    }

    HOSTDEV ~RGBImage() {
        CHECK_CUDA(cudaFree(buffer_));
    }

    HOSTDEV void clear() {
        thrust::fill(buffer_, buffer_ + w_ * h_, RGB{0, 0, 0});
    }


    DEV void setPixel(const Color &color, const int r, const int c) {
        const int i = r * w_ + c;
        buffer_[i].R = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.x)));
        buffer_[i].G = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.y)));
        buffer_[i].B = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.z)));
    }

    HOST void save(const char *path) const;

    HOSTDEV const RGB *data() const {
        return buffer_;
    }

    HOSTDEV int getW() const { return w_; }
    HOSTDEV int getH() const { return h_; }
private:
    // GPU version, no vector or resizing
    RGB *buffer_;
    int w_, h_;
};