#pragma once

#include "rt.hpp"
#include "scene.hpp"
#include "util/color.hpp"
#include "util/rand.hpp"

Vec3 integrate(Ray ray, const Scene &scene, int maxDepth, const Color &background, RNG &rng);
