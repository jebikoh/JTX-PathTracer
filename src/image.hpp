#pragma once

#include "rt.hpp"
#include "util/color.hpp"

constexpr Float MIN_INTENSITY = 0;
constexpr Float MAX_INTENSITY = 0.999;

inline Float linearToGamma(Float x) {
    if (x > 0) return jtx::sqrt(x);
    return 0;
}

static Float clampIntensity(Float i) {
    return jtx::clamp(i, MIN_INTENSITY, MAX_INTENSITY);
}

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

class RGB8Image {
public:
    int w_, h_;

    RGB8Image()
        : w_(1920),
          h_(1080){};
    RGB8Image(const int w, const int h)
        : w_(w),
          h_(h) {
        buffer.resize(w * h);
    }

    void resize(const int w, const int h) {
        w_ = w;
        h_ = h;
        buffer.resize(w * h);
    }

    void clear() {
        std::ranges::fill(buffer, RGB{0, 0, 0});
    }

    void setPixel(const Color &color, const int r, const int c) {
        const int i = r * w_ + c;
        buffer[i].R = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.r)));
        buffer[i].G = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.g)));
        buffer[i].B = static_cast<int>(RGB_SCALE * clampIntensity(linearToGamma(color.b)));
    }

    void save(const char *path) const;

    [[nodiscard]] const RGB *data() const {
        return buffer.data();
    }

private:
    std::vector<RGB> buffer;
};

class AccumulationBuffer {
public:
    int w_, h_;

    AccumulationBuffer()
        : w_(1920),
          h_(1080) {}
    AccumulationBuffer(const int w, const int h)
        : w_(w),
          h_(h) { buffer_.resize(w_ * h_); }

    void resize(const int w, const int h) {
        w_ = w;
        h_ = h;
        buffer_.resize(w * h);
    }
    void clear() { std::ranges::fill(buffer_, Vec3{0, 0, 0}); }

    Vec3 &updatePixel(const Vec3 &v, const int row, const int col) {
        const int i = row * w_ + col;
        buffer_[i] += v;
        return buffer_[i];
    }

    const Vec3 *data() const { return buffer_.data(); }

private:
    std::vector<Vec3> buffer_;
};

class TextureImage {
public:
    TextureImage()
        : width_(0),
          height_(0),
          channels_(0),
          data_(nullptr) {}

    explicit TextureImage(const char *path) { load(path); }

    TextureImage(TextureImage &&other) noexcept
        : isExr_(other.isExr_),
          path_(other.path_),
          width_(other.width_),
          height_(other.height_),
          channels_(other.channels_),
          data_(other.data_) {
        other.data_  = nullptr;
        other.path_  = "";
        other.width_ = other.height_ = other.channels_ = 0;
    }

    TextureImage(const TextureImage &) = delete;

    TextureImage &operator=(const TextureImage &) = delete;
    TextureImage &operator=(TextureImage &&other) noexcept;

    ~TextureImage();

    bool load(const char *path);

    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }

    const float *data() const { return data_; }

    Vec3 getTexel(const int u, const int v) const {
        int wrappedU = u % width_;
        if (wrappedU < 0) {
            wrappedU += width_;
        }

        int wrappedV = v % height_;
        if (wrappedV < 0) {
            wrappedV += height_;
        }

        const float *pixel = data_ + (wrappedV * width_ + wrappedU) * channels_;
        return Vec3(pixel[0], pixel[1], pixel[2]);
    }

    // Interpolated texel
    Vec3 getTexel(const float u, const float v) const {
        const int x0 = static_cast<int>(u * width_);
        const int y0 = static_cast<int>(v * height_);
        return getTexel(x0, y0);
    }

    Vec3 getTexel(const Vec2f &uv) const {
        return getTexel(uv.x, uv.y);
    }

private:
    bool isExr_;
    std::string path_;

    bool loadEXR(const char *path);

    int width_;
    int height_;
    int channels_;
    float *data_;
};