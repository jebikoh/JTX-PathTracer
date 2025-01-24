#pragma once

#include "rt.hpp"
#include "util/rand.hpp"
#include "util/color.hpp"

// void simplePathIntegrator(Ray &r, const BVHTree &world, int maxDepth, int &numRays, RNG &rng);
// void pathIntegrator();
Vec3 integrate(Ray ray, const BVHTree &world, int maxDepth, const Color &background, RNG &rng);
