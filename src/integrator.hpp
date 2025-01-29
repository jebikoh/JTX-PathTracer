#pragma once

#include "rt.hpp"
#include "scene.hpp"
#include "util/color.hpp"
#include "util/rand.hpp"

// void simplePathIntegrator(Ray &r, const BVHTree &world, int maxDepth, int &numRays, RNG &rng);
// void pathIntegrator();
Vec3 integrate(Ray ray, const Scene &scene, int maxDepth, const Color &background, RNG &rng);
