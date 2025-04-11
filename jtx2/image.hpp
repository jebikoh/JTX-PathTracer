#pragma once

#include "jtx.hpp"

#include <set>

namespace jtx {

static const std::set<std::string> JTX_IMAGE_SUPPORTED_FORMATS_8BIT  = {".jpeg", ".png", ".bmp", ".hdr", ".psd", ".tga", ".gif", ".pic", ".pgm", ".ppm"};
static const std::set<std::string> JTX_IMAGE_SUPPORTED_FORMATS_32BIT = {".jpeg", ".png", ".bmp", ".hdr", ".psd", ".tga", ".gif", ".pic", ".pgm", ".ppm", ".exr"};

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
public:
    int width, height, channels;

    /**
     * Creates an empty image (0x0x0)
     */
    Image8u()
        : width(0),
          height(0),
          channels(0) {}

    /**
     * Creates a zeroed (black) image of the given size and channel count
     * @param w width
     * @param h height
     * @param c channels
     */
    Image8u(const int w, const int h, const int c)
        : width(w),
          height(h),
          channels(c),
          m_data(w * h * c, 0) {}

    /**
     * Creates an image by copying data from a provided buffer
     * @param buffer 8-bit uint buffer
     * @param w buffer width
     * @param h buffer height
     * @param c buffer channels
     */
    Image8u(const uint8_t *buffer, int w, int h, int c);

    Image8u(Image8u &&image) noexcept;

    void destroy();

    /**
     * Loads an image from the given file. Accepts any format supported by stb. Note
     * float images will be converted to an 8-bit range (HDR->LDR)
     *
     * A failed load will return an empty image
     * @param path path to image file
     * @return 8-bit uint image
     */
    static Image8u loadImage(const std::filesystem::path &path);

    /**
     * Loads an image from a provided buffer
     * @param buffer 8-bit data buffer
     * @param size buffer size
     * @return 8-bit uint image
     */
    static Image8u loadImage(const uint8_t *buffer, size_t size);

    /**
     * Retrieves pixel value at given coordinates. If the requested format has more channels
     * than the image's format (RGB8u vs RGBA8u), the remaining channels will be set to 1
     *
     * This does not perform any bounds check--invalid coordinates will trigger a segfault
     * @tparam T pixel format. Must be a format of 8-bit or higher precision
     * @param row row
     * @param col column
     * @return pixel value at given coordinates in given format
     */
    template<typename T>
    T getPixel(int row, int col);

    const uint8_t *data() const { return m_data.data(); }
    uint8_t *data() { return m_data.data(); }

    const uint8_t &operator[](const int index) const { return m_data[index]; }
    uint8_t &operator[](const int index) { return m_data[index]; }

    bool isEmpty() const { return m_data.empty(); }

private:
    std::vector<uint8_t> m_data;
};

template<typename T>
T Image8u::getPixel(int row, int col) {
    LOG_ERROR(TEXTURE, "Attempted to retrieve pixel in unsupported format");
    return T::unimplemented;
}

template<>
inline RGB8u Image8u::getPixel<RGB8u>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {m_data[i], m_data[i + 1], m_data[i + 2]};
}

template<>
inline RGBA8u Image8u::getPixel<RGBA8u>(const int row, const int col) {
    const int i   = (row * width + col) * channels;
    RGBA8u result = {m_data[i], m_data[i + 1], m_data[i + 2], 1};
    if (channels == 4) { result.a = m_data[i + 3]; }
    return result;
}

template<>
inline RGB32f Image8u::getPixel<RGB32f>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {static_cast<float>(m_data[i]), static_cast<float>(m_data[i + 1]), static_cast<float>(m_data[i + 2])};
}

template<>
inline RGBA32f Image8u::getPixel<RGBA32f>(const int row, const int col) {
    const int i   = (row * width + col) * channels;
    RGBA32f result = {static_cast<float>(m_data[i]), static_cast<float>(m_data[i + 1]), static_cast<float>(m_data[i + 2]), 1.0f};
    if (channels == 4) { result.a = static_cast<float>(m_data[i + 3]); }
    return result;
}

/**
 * Represents an image with 32-bit float channels: RGBA32f and RGB32f
 */
class Image32f {
public:
    int width, height, channels;

    /**
     * Creates an empty image (0x0x0)
     */
    Image32f()
        : width(0),
          height(0),
          channels(0) {}

    /**
     * Creates a zeroed (black) image of the given size and channel count
     * @param w width
     * @param h height
     * @param c channels
     */
    Image32f(const int w, const int h, const int c)
        : width(w),
          height(h),
          channels(c),
          m_data(w * h * c, 0) {}

    /**
     * Creates an image by copying data from a provided buffer
     * @param buffer 32-bit float buffer
     * @param w buffer width
     * @param h buffer height
     * @param c buffer channels
     */
    Image32f(const float *buffer, int w, int h, int c);

    Image32f(Image32f &&image) noexcept;

    void destroy();

    /**
     * Loads an image from the given file. Accepts any format supported by stb as well
     * as OpenEXR files. Any LDR formats will be loaded as HDR
     * @param path path to image file
     * @return 32-bit float image
     */
    static Image32f loadImage(const std::filesystem::path &path);

    /**
     * Loads an image from a provided buffer
     * @param buffer 32-bit float buffer
     * @param size buffer size
     * @return 32-bit float image
     */
    static Image32f loadImage(const uint8_t *buffer, size_t size);

    /**
     * Retrieves pixel value at given coordinate. If the requested format has more channels
     * than the image's format (RGB32f vs RGBA32f), the remaining channels will be set to 1.0f
     *
     * This does not perform any bounds check--invalid coordinates will trigger a segfault
     * @tparam T pixel format. Must be a 32-bit float format (RGB32f or RGBA32f)
     * @param row row
     * @param col column
     * @return pixel value at given coordinates in given format
     */
    template<typename T>
    T getPixel(int row, int col);

    const float *data() const { return m_data.data(); }
    float *data() { return m_data.data(); }

    const float &operator[](const int index) const { return m_data[index]; }
    float &operator[](const int index) { return m_data[index]; }

    bool isEmpty() const { return m_data.empty(); }

private:
    std::vector<float> m_data;
};

template<typename T>
T Image32f::getPixel(int row, int col) {
    LOG_ERROR(TEXTURE, "Attempted to retrieve pixel in unsupported format");
    return T::unimplemented;
}

template<>
inline RGB32f Image32f::getPixel<RGB32f>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {m_data[i], m_data[i + 1], m_data[i + 2]};
}

template<>
inline RGBA32f Image32f::getPixel<RGBA32f>(const int row, const int col) {
    const int i    = (row * width + col) * channels;
    RGBA32f result = {m_data[i], m_data[i + 1], m_data[i + 2], 1.0f};
    if (channels == 4) { result.a = m_data[i + 3]; }
    return result;
}

}// namespace jtx