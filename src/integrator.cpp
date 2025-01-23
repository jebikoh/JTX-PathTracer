#include "integrator.hpp"
#include "material.hpp"
#include "util/interval.hpp"
#include "bvh.hpp"

Vec3 integrate(Ray &r, const BVHTree &world, const int maxDepth, const Color &background, RNG &rng) {
    Vec3 radiance;
    Vec3 beta;
    int depth = 0;

    Ray ray = r;

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

        // Sample BSDF
        BSDF bsdf = record.getBSDF();

        // Both w_o and w_i face outwards
        Vec3 w_o = -ray.dir;

        // Generate samples
        float u = rng.sampleFP();
        Vec2f u2 = rng.sampleVec2();

        // Sample BSDF
        BSDFSample sample = bsdf.sample(w_o, u, u2);

        // Update beta and set next ray
        beta *= sample.sample * jtx::absdot(sample.w_i, record.normal) / sample.pdf;
        ray = Ray(record.point, sample.w_i, record.t);
    }

    return radiance;
}