#pragma once
#include <jtx.hpp>

namespace jtx {
class Sampler;

vec3 Integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

}// namespace jtx