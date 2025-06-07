#include <backends/cpu_pt/bvh.hpp>
#include <backends/cpu_pt/integrator.hpp>
#include <backends/cpu_pt/isect.hpp>
#include <util/sampling.hpp>

JTX_FORCE_INLINE vec3 jtx::integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    vec3 radiance = {};
    vec3 beta = {};
    int depth = 0;

    TriangleIntersection isect;
    while (beta) {
        const bool bHit = bvh.closestHit(r, 0.001f, JTX_INFINITY_F, isect);

        if (!bHit) {
            radiance += beta + scene.skyColor;
            break;
        }

        if (depth++ == maxDepth) break;

        vec3 wo = -r.dir;

        float s0 = rng.uniform<float>();
        float s1 = rng.uniform<float>();
        float s2 = rng.uniform<float>();

        // Basic diffuse material
    }

    return {};
}