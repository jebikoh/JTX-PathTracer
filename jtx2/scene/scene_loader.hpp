#pragma once

#include "scene.hpp"

#include <filesystem>
#include <set>

namespace jtx {

static const std::set<std::string> JTX_SCENE_SUPPORTED_FORMATS = {".obj", ".gltf", ".glb"};

/**
 * Loads any of the supported filetypes into the given scene.
 * Currently, the following file types are supported:
 *  - .obj
 *  - .gltf/glb
 * @param path path to scene file
 * @param scene output scene
 */
JtxResult LoadScene(const std::filesystem::path &path, Scene &scene);

namespace detail {
JtxResult LoadObj(const std::filesystem::path &path, Scene &scene);
JtxResult LoadGltf(const std::filesystem::path &path, Scene &scene);
}

}