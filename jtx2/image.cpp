#include "image.hpp"

#include <filesystem>

jtx::Image8u jtx::Image8u::loadImage(const std::filesystem::path &path) {
    LOG_INFO("Loading 8-bit texture: {}", path.string());

    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_IMAGE_SUPPORTED_FORMATS.contains(fileExt)) {
        LOG_ERROR("Unsupported image format: {}", fileExt);
        return {};
    }

    if (fileExt == "exr") {

    }


}
