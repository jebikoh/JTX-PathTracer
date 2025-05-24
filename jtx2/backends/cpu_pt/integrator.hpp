#pragma once
#include <jtx.hpp>

namespace jtx {
class RNG;

vec3 integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, RNG &rng);

}// namespace jtx