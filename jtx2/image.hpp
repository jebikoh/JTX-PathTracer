#pragma once

#include "jtx.hpp"

#include <filesystem>
#include <set>

namespace jtx {

static const std::set<std::string> JTX_IMAGE_SUPPORTED_FORMATS_8BIT = {
        ".jpeg", ".jpg", ".png", ".bmp", ".hdr", ".psd", ".tga", ".gif", ".pic", ".pgm", ".ppm"};
static const std::set<std::string> JTX_IMAGE_SUPPORTED_FORMATS_32BIT = {
        ".jpeg", ".jpg", ".png", ".bmp", ".hdr", ".psd", ".tga", ".gif", ".pic", ".pgm", ".ppm", ".exr"};

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

// Macros for usage in tight-loops
#define JTX_IMAGE_ROW_STRIDE(img) \
    ((img).width * (img).channels)
#define JTX_IMAGE_ROW_PTR(img, row) \
    ((img).pData + (row) * JTX_IMAGE_ROW_STRIDE(img))
#define JTX_IMAGE_PIXEL_PTR(img, row, col) \
    (JTX_IMAGE_ROW_PTR(img, row) + (col) * (img).channels)

#define JTX_IMAGE_PIXEL_R (img, row, col)(JTX_IMAGE_PIXEL_PTR(img, row, col)[0])
#define JTX_IMAGE_PIXEL_G (img, row, col)(JTX_IMAGE_PIXEL_PTR(img, row, col)[1])
#define JTX_IMAGE_PIXEL_B (img, row, col)(JTX_IMAGE_PIXEL_PTR(img, row, col)[2])
#define JTX_IMAGE_PIXEL_A (img, row, col)(JTX_IMAGE_PIXEL_PTR(img, row, col)[3])
#define JTX_IMAGE_PIXEL_A_SAFE (img, row, col)(img.channels == 4 ? JTX_IMAGE_PIXEL_PTR(img, row, col)[3] : 1)

/**
 * Represents an image with 8-bit unsigned integer channels: RGBA8u and RGB8u
 */
class Image8u {
public:
    int width, height, channels;
    uint8_t *pData;

    /**
     * Creates an empty image (0x0x0)
     */
    Image8u()
        : width(0),
          height(0),
          channels(0),
          pData(nullptr) {
    }

    /**
     * Creates a zeroed (black) image of the given size and channel count. Allocates memory.
     * @param w width
     * @param h height
     * @param c channels
     */
    Image8u(const int w, const int h, const int c)
        : width(w),
          height(h),
          channels(c) {
        pData = new uint8_t[w * h * c];
    }

    /**
     * Creates an image by copying data from a provided buffer
     * @param buffer 8-bit uint buffer
     * @param w buffer width
     * @param h buffer height
     * @param c buffer channels
     */
    Image8u(const uint8_t *buffer, int w, int h, int c);

    Image8u(Image8u &&other) noexcept;
    Image8u &operator=(Image8u &&other) noexcept;

    void Destroy();

    /**
     * Loads an image from the given file. Accepts any format supported by stb. Note
     * float images will be converted to an 8-bit range (HDR->LDR).
     * @param path path to image file
     * @param out output image
     * @param bApplyEOTF will apply sRGB EOTF function if true
     * @return JTX_SUCCESS if successful, failure otherwise
     */
    static JtxResult Load(const std::filesystem::path &path, Image8u &out, bool bApplyEOTF = true);

    /**
     * Loads an image from a provided buffer.
     * @param buffer 8-bit data buffer
     * @param size buffer size
     * @param out output image
     * @param bApplyEOTF will apply sRGB EOTF function if true
     * @return JTX_SUCCESS if successful, failure otherwise
     */
    static JtxResult Load(const uint8_t *buffer, size_t size, Image8u &out, bool bApplyEOTF = true);

    JtxResult Save(const std::filesystem::path &path, bool bFlip = true) const;

    /**
     * Retrieves pixel value at given coordinates. If the requested format has more channels
     * than the image's format (RGB8u vs RGBA8u), the remaining channels will be set to 1.
     *
     * Using a wider pixel format (RGB32f, RGBA32f) will cast the values to the requested format
     *
     * This does not perform any bounds check--invalid coordinates will trigger a segfault
     * If you are retrieving data in a tight-loop, consider using the pointer macros instead
     * @tparam T pixel format. Must be a format of 8-bit or higher precision
     * @param row row
     * @param col column
     * @return pixel value at given coordinates in given format
     */
    template<typename T>
    T GetPixel(int row, int col);

    const uint8_t &operator[](const int index) const { return pData[index]; }
    uint8_t &operator[](const int index) { return pData[index]; }

    bool IsEmpty() const { return width == 0 || height == 0 || channels == 0; }

    /**
     * Returns a copy of the image as a 32-bit image, expanding the format if needed.
     * @return
     */
    Image8u As32b(uint8_t alpha = 255) const;

    /**
     * Resizes the current images -- data is not copied
     * @param w new width
     * @param h new height
     * @param c new channels
     */
    void Resize(const uint32_t w, const uint32_t h, const uint32_t c) {
        this->Destroy();
        width = w;
        height = h;
        channels = c;
        pData = new uint8_t[w * h * c]();
    }
};

template<typename T>
T Image8u::GetPixel(int row, int col) {
    LOG_ERROR(TEXTURE, "Attempted to retrieve pixel in unsupported format");
    return T::unimplemented;
}

template<>
inline RGB8u Image8u::GetPixel<RGB8u>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {pData[i], pData[i + 1], pData[i + 2]};
}

template<>
inline RGBA8u Image8u::GetPixel<RGBA8u>(const int row, const int col) {
    const int i   = (row * width + col) * channels;
    RGBA8u result = {pData[i], pData[i + 1], pData[i + 2], 1};
    if (channels == 4) { result.a = pData[i + 3]; }
    return result;
}

template<>
inline RGB32f Image8u::GetPixel<RGB32f>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {static_cast<float>(pData[i]), static_cast<float>(pData[i + 1]), static_cast<float>(pData[i + 2])};
}

template<>
inline RGBA32f Image8u::GetPixel<RGBA32f>(const int row, const int col) {
    const int i    = (row * width + col) * channels;
    RGBA32f result = {
            static_cast<float>(pData[i]), static_cast<float>(pData[i + 1]), static_cast<float>(pData[i + 2]), 1.0f};
    if (channels == 4) { result.a = static_cast<float>(pData[i + 3]); }
    return result;
}

/**
 * Represents an image with 32-bit float channels: RGBA32f and RGB32f
 */
class Image32f {
public:
    int width, height, channels;
    float *pData;

    /**
     * Creates an empty image (0x0x0)
     */
    Image32f()
        : width(0),
          height(0),
          channels(0),
          pData(nullptr) {
    }

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
          pData(nullptr) {
    }

    /**
     * Creates an image by copying data from a provided buffer
     * @param buffer 32-bit float buffer
     * @param w buffer width
     * @param h buffer height
     * @param c buffer channels
     */
    Image32f(const float *buffer, int w, int h, int c);

    Image32f(Image32f &other);
    Image32f &operator=(Image32f &other);

    void Destroy();

    /**
     * Loads an image from the given file. Accepts any format supported by stb as well
     * as OpenEXR files. Any LDR formats will be loaded as HDR
     * @param path path to image file
     * @param out output image
     * @return JTX_SUCCESS if successful, failure otherwise
     */
    static JtxResult Load(const std::filesystem::path &path, Image32f &out);

    /**
     * Loads an image from a provided buffer
     * @param buffer 32-bit float buffer
     * @param size buffer size
     * @param out output image
     * @return JTX_SUCCESS if successful, failure otherwise
     */
    static JtxResult Load(const uint8_t *buffer, size_t size, Image32f &out);

    /**
     * Retrieves pixel value at given coordinate. If the requested format has more channels
     * than the image's format (RGB32f vs RGBA32f), the remaining channels will be set to 1.0f
     *
     * This does not perform any bounds check--invalid coordinates will trigger a segfault
     * If you are retrieving data in a tight-loop, consider using the pointer macros instead
     * @tparam T pixel format. Must be a 32-bit float format (RGB32f or RGBA32f)
     * @param row row
     * @param col column
     * @return pixel value at given coordinates in given format
     */
    template<typename T>
    T GetPixel(int row, int col);

    const float &operator[](const int index) const { return pData[index]; }
    float &operator[](const int index) { return pData[index]; }

    bool IsEmpty() const { return width == 0 || height == 0 || channels == 0; }

    /**
     * Resizes the current images -- data is not copied
     * @param w new width
     * @param h new height
     * @param c new channels
     */
    void Resize(const uint32_t w, const uint32_t h, const uint32_t c) {
        this->Destroy();
        width = w;
        height = h;
        channels = c;
        pData = new float[w * h * c]();
    }
};

template<typename T>
T Image32f::GetPixel(int row, int col) {
    LOG_ERROR(TEXTURE, "Attempted to retrieve pixel in unsupported format");
    return T::unimplemented;
}

template<>
inline RGB32f Image32f::GetPixel<RGB32f>(const int row, const int col) {
    const int i = (row * width + col) * channels;
    return {pData[i], pData[i + 1], pData[i + 2]};
}

template<>
inline RGBA32f Image32f::GetPixel<RGBA32f>(const int row, const int col) {
    const int i    = (row * width + col) * channels;
    RGBA32f result = {pData[i], pData[i + 1], pData[i + 2], 1.0f};
    if (channels == 4) { result.a = pData[i + 3]; }
    return result;
}

/**
 * Calculates the Mean Squared Error (MSE) between two images.
 *
 * Images must have the same dimensions and channel count.
 * @param img the image to compare
 * @param ref the reference image to compare against
 * @return MSE value, or -1.0f on failure.
 */
float CalculateMSE(const Image8u &img, const Image8u &ref);

/**
 * Loads images and calculates the Mean Squared Error (MSE) between them.
 *
 * Images must have the same dimensions and channel count.
 * @param imgPath path to the image to compare
 * @param refPath path to the reference image to compare against
 * @return MSE value, or -1.0f on failure
 */
inline float CalculateMSE(const std::filesystem::path &imgPath, const std::filesystem::path &refPath) {
    Image8u img;
    if (Image8u::Load(imgPath, img) != JTX_SUCCESS) {
        LOG_ERROR(TEXTURE, "Failed to load image");
        return -1.0f;
    }

    Image8u ref;
    if (Image8u::Load(refPath, ref) != JTX_SUCCESS) {
        LOG_ERROR(TEXTURE, "Failed to load reference image");
        return -1.0f;
    }

    const float mse = CalculateMSE(img, ref);

    img.Destroy();
    ref.Destroy();
    return mse;
}

}// namespace jtx
