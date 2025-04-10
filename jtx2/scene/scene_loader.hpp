#pragma once

#include "scene.hpp"

#include <filesystem>

namespace jtx {

/**
 * Loads any of the supported filetypes into the given scene.
 * Currently, the following file types are supported:
 *  - .obj
 *  - .gltf/glb
 * @param path path to scene file
 * @param scene output scene
 */
bool loadScene(const std::filesystem::path &path, Scene &scene);

namespace detail {
bool loadObj(const std::filesystem::path &path, Scene &scene);
bool loadGltf(const std::filesystem::path &path, Scene &scene);
}

}