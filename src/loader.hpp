#pragma once
#include <string>

class Scene;

bool loadScene(const std::string &path, Scene &scene);
bool loadObj(const std::string &path, Scene &scene);
bool loadGltf(const std::string &path, Scene &scene);