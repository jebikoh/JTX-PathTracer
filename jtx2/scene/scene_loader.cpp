#include "scene_loader.hpp"

#include <set>

static const std::set<std::string> JTX_SUPPORTED_FILE_EXTENSIONS = {"obj", "gltf", "glb"};

bool jtx::loadScene(const std::filesystem::path &path, Scene &scene) {
    LOG_INFO("Loading scene: {}", path.c_str());
    auto fileExt = path.extension().string();
    std::ranges::transform(fileExt, fileExt.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (!JTX_SUPPORTED_FILE_EXTENSIONS.contains(fileExt)) {
        LOG_ERROR("File extension not supported: ", fileExt);
        return false;
    }

    if (fileExt == "obj") {
        return detail::loadObj(path, scene);
    }

    if (fileExt == "gltf") {
        return detail::loadGltf(path, scene);
    }

    LOG_INFO("Scene loaded");
    return true;
}