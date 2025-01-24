#include "integrator.hpp"
#include "material.hpp"
#include "util/interval.hpp"
#include "bvh.hpp"
#include "bxdf.hpp"

Vec3 integrate(Ray ray, const BVHTree &world, const int maxDepth, const Color &background, RNG &rng) {
    Vec3 radiance = {};
    Vec3 beta = {1, 1, 1};
    int depth = 0;

    HitRecord record;
    while (beta) {
        const bool hit = world.hit(ray, Interval(0.001, INF), record);

        if (!hit) {
            radiance += beta * background;
            break;
        }
        // Emission (L_e)
        // PBRT checks for specular bounce
        radiance += beta * record.material->emission;

        // Depth exceeded
        if (depth++ == maxDepth) break;

        // Both w_o and w_i face outwards
        Vec3 w_o = -ray.dir;

        // TODO: light sampling (once we add lights)

        // Generate samples
        float u = rng.sampleFP();
        Vec2f u2 = rng.sampleVec2();

        // Sample BSDF
        auto [fSample, w_i, pdf] = sampleBxdf(record.material, record, w_o, u, u2);

        // Update beta and set next ray
        beta *= fSample * jtx::absdot(w_i, record.normal) / pdf;
        ray = Ray(record.point, w_i, record.t);
    }

    return radiance;
}