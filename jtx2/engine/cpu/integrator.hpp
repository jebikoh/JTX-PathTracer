#pragma once
#include <jtx.hpp>

namespace jtx {
class Sampler;

constexpr uint32_t JTX_RR_MIN_DEPTH = 1;

vec3 Integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

vec3 IntegrateRR(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

vec3 IntegrateNEE(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

}// namespace jtx