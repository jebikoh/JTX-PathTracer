#include "integrator.hpp"
#include "bsdf/bxdf.hpp"
#include "material.hpp"
#include "util/interval.hpp"

Vec3 integrate(Ray ray, const Scene &scene, const int maxDepth, const Color &background, RNG &rng) {
    Vec3 radiance       = {};
    Vec3 beta           = {1, 1, 1};
    int depth           = 0;
    bool specularBounce = true;
    HitRecord record;

    while (beta) {
        const bool hit = scene.hit(ray, Interval(0.001, INF), record);

        if (!hit) {
            if (specularBounce) {
                radiance += beta * background;
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
            auto lightIdx      = rng.sampleRange(scene.lights.size());
            const Light &light = scene.lights[lightIdx];

            LightSample ls;
            LightSampleContext ctx;
            ctx.p  = record.point;
            ctx.n  = record.normal;
            ctx.sn = record.normal;

            bool lightSampled = sampleLight(light, ctx, ls);
            if (lightSampled && ls.pdf > 0) {
                Vec3 w_i = ls.wi;
                auto f   = evalBxdf(record.material, record, w_o, w_i) * jtx::absdot(w_i, ctx.sn);
                // TODO: add occlusion test
                if (f) {
                    radiance += beta * f * ls.radiance / (ls.pdf * (1 / scene.lights.size()));
                }
            }
        }


        // Generate samples
        float u  = rng.sample<float>();
        Vec2f u2 = rng.sample<Vec2f>();

        // Sample BSDF
        BSDFSample s;
        bool success = sampleBxdf(record.material, record, w_o, u, u2, s);
        if (!success) break;

        // Update beta and set next ray
        beta *= s.fSample * jtx::absdot(s.w_i, record.normal) / s.pdf;
        specularBounce = s.isSpecular;

        ray = Ray(record.point, s.w_i, record.t);
    }

    return radiance;
}

//bool visible(const Vec3 &p1, const Vec3 &p2, const Scene &scene) {
//    Vec3 dir = p2 - p1;
//}
