#pragma once

#include "rt.hpp"
#include "scene.hpp"
#include "util/color.hpp"
#include "util/rand.hpp"

bool visible(const Vec3 &p1, const Vec3 &p2, const Scene &scene);

Vec3 integrate(Ray ray, const Scene &scene, int maxDepth, const Color &background, RNG &rng);
