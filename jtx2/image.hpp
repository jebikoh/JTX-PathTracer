#pragma once

#include "jtx.hpp"

#include <set>

namespace jtx {

static const std::set<std::string> JTX_IMAGE_SUPPORTED_FORMATS = {"jpeg", "png", "bpm", "hdr", "psd", "tga", "gif", "pic", "psd", "pgm", "ppm", "exr"};

// Image formats
struct RGBA8u {
    uint8_t r, g, b, a;
};

struct RGB8u {
    uint8_t r, g, b;
};

struct RGBA32f {
    float r, g, b, a;
};

struct RGB32f {
    float r, g, b;
};

// Image classes

/**
 * Represents an image with 8-bit unsigned integer channels: RGBA8u and RGB8u
 */
class Image8u {
    int width, height, channels;

    /**
     * Creates an empty image (0x0x0)
     */
    Image8u() : width(0), height(0), channels(0) {}

    /**
     * Creates an zeroed (black) image of the given size and channel count
     * @param w width
     * @param h height
     * @param c channels
     */
    Image8u(int w, int h, int c) : width(w), height(h), channels(c), m_data(w * h * c, 0) {}

    /**
     * Loads an image from the given file. Accepts any format supported by stb and .EXR files
     *
     * A failed load will return an empty image
     * @param path path to image file
     * @return 8-bit uint image
     */
    static Image8u loadImage(const std::filesystem::path &path);

    bool isEmpty() const { return m_data.empty(); }
private:
    std::vector<uint8_t> m_data;

    static Image8u loadStbi(const std::filesystem::path &path);
    static Image8u loadExr(const std::filesystem::path &path);
};

/**
 * Represents an image with 32-bit float channels: RGBA32f and RGB32f
 */
class Image32f {
    int width, height, channels;


private:
    std::vector<float> m_data;
};

}// namespace jtx