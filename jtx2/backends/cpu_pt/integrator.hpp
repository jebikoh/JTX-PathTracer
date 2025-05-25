#pragma once
#include <jtx.hpp>

namespace jtx {
class Sampler;

vec3 integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

}// namespace jtx