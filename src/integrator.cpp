#include "integrator.hpp"
#include "bsdf/bxdf.hpp"
#include "material.hpp"
#include "util/interval.hpp"

float powerHeuristic(const float nf, const float fPdf, const float ng, const float gPdf) {
    const float f = nf * fPdf;
    const float g = ng * gPdf;
    return f * f / (f * f + g * g);
}

Vec3 integrateBasic(Ray ray, const Scene &scene, int maxDepth, RNG &rng) {
    Vec3 radiance = {};
    Vec3 beta     = {1, 1, 1};
    int depth     = 0;

    SurfaceIntersection record;
    while (beta) {
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            radiance += beta * scene.skyColor;
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
        float u  = rng.sample<float>();
        Vec2f u2 = rng.sample<Vec2f>();

        // Sample BSDF
        BSDFSample s;
        bool success = sampleBxdf(scene, record, w_o, u, u2, s);
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
    SurfaceIntersection record;

    while (beta) {
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            if (specularBounce) {
                radiance += beta * scene.skyColor;
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
            auto lightIdx = rng.sampleRange(scene.lights.size() - 1);
            //            std::cout << "Light index: " << lightIdx << std::endl;
            const Light &light = scene.lights[lightIdx];

            LightSample ls;
            LightSampleContext ctx;
            ctx.p = record.point;
            ctx.n = record.normal;
            // ctx.sn = record.normal;

            Vec2f u = rng.sample<Vec2f>();

            bool lightSampled = light.sample(ctx, ls, u);
            if (lightSampled && ls.pdf > 0) {
                Vec3 w_i = ls.wi;
                auto f   = evalBxdf(scene, record.material, record, w_o, w_i) * jtx::absdot(w_i, ctx.n);

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
        bool success = sampleBxdf(scene, record, w_o, u, u2, s);
        if (!success) break;

        // Update beta and set next ray
        beta *= s.fSample * jtx::absdot(s.w_i, record.normal) / s.pdf;
        specularBounce = s.isSpecular;

        ray = Ray(record.point + s.w_i * RAY_EPSILON, s.w_i, record.t);
    }

    return radiance;
}

Vec3 sampleLights(const Ray &r, const Scene &scene, const SurfaceIntersection &record, RNG &rng, LightSample &ls) {
    const auto lightIdx = rng.sampleRange(scene.lights.size() - 1);
    const Light &light  = scene.lights[lightIdx];

    LightSampleContext ctx;
    ctx.p = record.point;
    ctx.n = record.normal;

    const auto u = rng.sample<Vec2f>();

    if (light.sample(ctx, ls, u)) {
        // Shadow ray
        const Vec3 sOrigin = record.point + record.normal * RAY_EPSILON;
        const auto sRay    = Ray(sOrigin, ls.wi);
        const auto lDist   = jtx::distance(record.point, ls.p);

        const bool occluded = scene.anyHit(sRay, Interval(0.0f, lDist - RAY_EPSILON));
        if (!occluded) {
            const auto wo = -r.dir;
            const auto wi = ls.wi;

            const auto f = evalBxdf(scene, record.material, record, wo, wi) * jtx::absdot(wi, ctx.n);
            float pb     = pdfBxdf(scene, record.material, record, wo, wi);
            float pl     = 1.0f / static_cast<float>(scene.lights.size()) * ls.pdf;

            float misWeight = 1.0f;
            if (light.type != Light::POINT || light.type != Light::DISTANT) {
                misWeight = powerHeuristic(1, pl, 1, pb);
            }

            return misWeight * f * ls.radiance / pl;
        }
    }

    return {};
}

Vec3 integrateMIS(Ray ray, const Scene &scene, int maxDepth, bool regularize, RNG &rng) {
    Vec3 radiance = {};
    Vec3 beta     = {1, 1, 1};
    int depth     = 0;
    SurfaceIntersection record;
    LightSample lightSample;

    bool hasLights = !scene.lights.empty();

    while (true) {
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            // Sky color is not importance sampled (for now)
            radiance += beta * scene.skyColor;
            break;
        }

        // Check for emission/if we hit a light
        // TODO: don't support emission for now
        if (depth++ == maxDepth) break;

        // Light sampling
        if (hasLights) {
            radiance += beta * sampleLights(ray, scene, record, rng, lightSample);
        }

        // BxDF sampling
        Vec3 wo       = -ray.dir;
        const float u = rng.sample<float>();
        const auto u2 = rng.sample<Vec2f>();

        BSDFSample s;
        bool success = sampleBxdf(scene, record, wo, u, u2, s);
        if (!success) break;

        // Update beta and set next ray
        if (s.pdf > 0.0f) {
            beta *= s.fSample * jtx::absdot(s.w_i, record.normal) / s.pdf;
        }

        ray = Ray(record.point + s.w_i * RAY_EPSILON, s.w_i, record.t);
    }

    return radiance;
}
