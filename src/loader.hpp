#pragma once
#include "fastgltf/types.hpp"
#include "image.hpp"

class Scene;

bool loadScene(const std::string &path, Scene &scene);
bool loadObj(const std::string &path, Scene &scene);
bool loadGltf(const std::filesystem::path &path, Scene &scene);

namespace detail {

std::optional<TextureImage> loadTexture(fastgltf::Asset &asset, fastgltf::Image &image);

}