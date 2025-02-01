#include "integrator.hpp"
#include "bsdf/bxdf.hpp"
#include "material.hpp"
#include "util/interval.hpp"

Vec3 integrateBasic(Ray ray, const Scene &scene, int maxDepth, RNG &rng) {
    Vec3 radiance = {};
    Vec3 beta = {1, 1, 1};
    int depth = 0;

    HitRecord record;
    while (beta) {
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            radiance += beta * scene.lights[0].evaluate(ray);
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
        float u = rng.sample<float>();
        Vec2f u2 = rng.sample<Vec2f>();

        // Sample BSDF
        BSDFSample s;
        bool success = sampleBxdf(record.material, record, w_o, u, u2, s);
        if (!success) break;

        // Update beta and set next ray
        beta *= s.fSample * jtx::absdot(s.w_i, record.normal) / s.pdf;
        ray = Ray(record.point + s.w_i * RAY_EPSILON, s.w_i, record.t);
    }

    return radiance;
}

Vec3 integrate(Ray ray, const Scene &scene, const int maxDepth, RNG &rng) {
    Vec3 radiance       = {};
    Vec3 beta           = {1, 1, 1};
    int depth           = 0;
    bool specularBounce = true;
    HitRecord record;

    while (beta) {
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            if (specularBounce && scene.lights[0].type == Light::INFINITE) {
                radiance += beta * scene.lights[0].evaluate(ray);
            }
            break;
        }

        // Emission (L_e)
        // Only do this in case of a specular bounce
        // We account for emission via light sampling
        if (specularBounce) {
            radiance += beta * record.material->emission;
        }

        // Depth exceeded
        if (depth++ == maxDepth) break;

        // Both w_o and w_i face outwards
        Vec3 w_o = -ray.dir;

        {// Light sampling
            auto lightIdx      = rng.sampleRange(scene.lights.size() - 1);
//            std::cout << "Light index: " << lightIdx << std::endl;
            const Light &light = scene.lights[lightIdx];

            LightSample ls;
            LightSampleContext ctx;
            ctx.p  = record.point;
            ctx.n  = record.normal;
            ctx.sn = record.normal;

            Vec2f u = rng.sample<Vec2f>();

            bool lightSampled = light.sample(ctx, ls, u);
            if (lightSampled && ls.pdf > 0) {
                Vec3 w_i = ls.wi;
                auto f   = evalBxdf(record.material, record, w_o, w_i) * jtx::absdot(w_i, ctx.sn);

                // Occlusion
                // Offset ray from origin along normal to avoid self-collisions
                const Vec3 sOrigin = record.point + record.normal * RAY_EPSILON;
                const auto sRay    = Ray(sOrigin, ls.wi);
                // Calculate distance for t parameter
                const auto lDist = jtx::distance(record.point, ls.p);

                if (f && !scene.anyHit(sRay, Interval(0.0f, lDist - RAY_EPSILON))) {
                    radiance += beta * f * ls.radiance / (ls.pdf * (1.0f / static_cast<float>(scene.lights.size())));
                }
            }
        }

        // Generate samples
        float u = rng.sample<float>();
        auto u2 = rng.sample<Vec2f>();

        // Sample BSDF
        BSDFSample s;
        bool success = sampleBxdf(record.material, record, w_o, u, u2, s);
        if (!success) break;

        // Update beta and set next ray
        beta *= s.fSample * jtx::absdot(s.w_i, record.normal) / s.pdf;
        specularBounce = s.isSpecular;

        ray = Ray(record.point + s.w_i * RAY_EPSILON, s.w_i, record.t);
    }

    return radiance;
}
