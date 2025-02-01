#pragma once

#include "rt.hpp"
#include "scene.hpp"
#include "util/color.hpp"
#include "util/rand.hpp"

Vec3 integrateBasic(Ray ray, const Scene &scene, int maxDepth, RNG &rng);

Vec3 integrate(Ray ray, const Scene &scene, int maxDepth, RNG &rng);

Vec3 integrateMIS();