#pragma once

#include "rt.hpp"
#include "util/rand.hpp"
#include "util/color.hpp"

// void simplePathIntegrator(Ray &r, const BVHTree &world, int maxDepth, int &numRays, RNG &rng);
void pathIntegrator();
Vec3 integrate(Ray &r, const BVHTree &world, const int maxDepth, const Color &background, RNG &rng);
