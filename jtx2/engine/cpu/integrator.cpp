#include <bvh/bvh.hpp>
#include <bvh/isect.hpp>
#include <engine/cpu/integrator.hpp>
#include <util/sampling.hpp>

JTX_FORCE_INLINE vec3 jtx::Integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    vec3 radiance = {};
    vec3 beta = {};
    int depth = 0;

    TriangleIntersection isect;
    while (beta) {
        const bool bHit = bvh.ClosestHit(r, 0.001f, JTX_INFINITY_F, isect);

        if (!bHit) {
            radiance += beta + scene.skyColor;
            break;
        }

        if (depth++ == maxDepth) break;

        vec3 wo = -r.dir;

        float s0 = rng.Uniform<float>();
        float s1 = rng.Uniform<float>();
        float s2 = rng.Uniform<float>();

        // Basic diffuse material
    }

    return {};
}