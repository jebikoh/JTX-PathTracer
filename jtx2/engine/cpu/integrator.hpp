#pragma once
#include <jtx.hpp>

namespace jtx {
class Sampler;

constexpr uint32_t JTX_RR_MIN_DEPTH = 1;

inline float PowerHeuristic(const float fPDF, const float gPDF, const int nf = 1, const int ng = 1) {
    const float f = nf * fPDF;
    const float g = ng * gPDF;
    return (f * f) / (f * f + g * g);
}

inline float BalanceHeuristic(const float fPDF, const float gPDF, const int nf = 1, const int ng = 1) {
    const float f = nf * fPDF;
    const float g = ng * gPDF;
    return f / (f + g);
}

vec3 Integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

vec3 IntegrateRR(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

vec3 IntegrateNEE(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

// NEE + MIS
vec3 IntegrateMIS(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng);

}// namespace jtx