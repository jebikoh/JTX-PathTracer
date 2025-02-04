#include "integrator.hpp"
#include "bsdf/bxdf.hpp"
#include "material.hpp"
#include "util/interval.hpp"
#include "bsdf/microfacet.hpp"

float powerHeuristic(float nf, float fPdf, float ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    return f * f / (f * f + g * g);
}

Vec3 integrateBasic(Ray ray, const Scene &scene, int maxDepth, RNG &rng) {
    Vec3 radiance = {};
    Vec3 beta     = {1, 1, 1};
    int depth     = 0;

    Intersection record;
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
        float u  = rng.sample<float>();
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
    Intersection record;

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
            auto lightIdx = rng.sampleRange(scene.lights.size() - 1);
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

bool isNonSpecular(const Material *mat) {
    if (mat->type == Material::Type::CONDUCTOR || mat->type == Material::Type::DIELECTRIC) {
        return jtx::max(mat->alphaX, mat->alphaY) > TR_SMOOTH_THRESHOLD;
    }
    return true;
}

Vec3 integrateMIS(Ray ray, const Scene &scene, int maxDepth, bool regularize, RNG &rng) {
    Vec3 radiance             = {};
    Vec3 beta                 = {1, 1, 1};
    int depth                 = 0;
    float pbPrev              = 1.0f;
    float etaScale            = 1.0f;
    bool anyNonSpecularBounce = false;
    bool specularBounce       = true;
    Intersection record;
    LightSampleContext prevCtx;

    while (true) {
        // Cast ray & find the closest hit
        const bool hit = scene.closestHit(ray, Interval(0.001, INF), record);

        if (!hit) {
            // Incorporate infinite lights and break
            if (scene.lights[0].type == Light::INFINITE) {
                const auto light = scene.lights[0];

                auto L = light.evaluate(ray);
                if (depth == 0 || specularBounce) {
                    radiance += beta * L;
                } else {
                    // Use MIS
                    float pl = 1.0f / static_cast<float>(scene.lights.size()) * light.pdf(prevCtx, ray.dir, true);
                    float wb = powerHeuristic(1, pl, 1, pbPrev);
                    radiance += beta * wb * L;
                }
            }
            break;
        }

        // TODO: check for emission (we don't support emission yet)

        // TODO: Regularize

        // Check depth
        if (depth++ == maxDepth) break;

        // Sample direct illumination if non-specular
        if (isNonSpecular(record.material)) {
            LightSample ls;
            LightSampleContext ctx;
            ctx.p  = record.point;
            ctx.n  = record.normal;
            ctx.sn = record.normal;

            // Uniformly sample a light
            float u            = rng.sample<float>();
            int lightIndex     = jtx::min<int>(u * scene.lights.size(), scene.lights.size() - 1);
            const Light &light = scene.lights[lightIndex];

            // Sample the light
            bool lightSampled = light.sample(ctx, ls, rng.sample<Vec2f>());
            if (lightSampled && ls.pdf > 0 && ls.radiance) {
                // Evaluate BSDF for light sample
                Vec3 wo = -ray.dir;
                Vec3 wi = ls.wi;
                Vec3 f  = evalBxdf(record.material, record, wo, wi) * jtx::absdot(wi, ctx.sn);

                // Calculate shadow ray
                const Vec3 sOrigin = record.point + record.normal * RAY_EPSILON;
                const auto sRay    = Ray(sOrigin, ls.wi);
                const auto lDist   = jtx::distance(record.point, ls.p);

                // Check for occlusion
                if (f && !scene.anyHit(sRay, Interval(0.0f, lDist - RAY_EPSILON))) {
                    float pl = 1.0f / static_cast<float>(scene.lights.size()) * ls.pdf;
                    if (light.type == Light::POINT) {
                        // Delta distribution
                        radiance += beta * ls.radiance * f / pl;
                    } else {
                        // MIS
                        float pb     = pdfBxdf(record.material, record, wo, wi);
                        float weight = powerHeuristic(1, ls.pdf * pl, 1, pb);
                        radiance += beta * weight * ls.radiance * f / pl;
                    }
                }
            }
        }

        // Sample BSDF for next ray
        Vec3 wo = -ray.dir;
        float u = rng.sample<float>();
        BSDFSample bs;
        bool success = sampleBxdf(record.material, record, wo, u, rng.sample<Vec2f>(), bs);
        if (!success) break;

        // Update variables
        beta *= bs.fSample * jtx::absdot(bs.w_i, record.normal) / bs.pdf;
        pbPrev = bs.pdf;
        specularBounce = bs.isSpecular;
        anyNonSpecularBounce |= !bs.isSpecular;
        if (bs.isTransmission) etaScale *= bs.eta * bs.eta;
        prevCtx = {record.point, record.normal, record.normal};

        // Set next ray
        ray = Ray(record.point + bs.w_i * RAY_EPSILON, bs.w_i, record.t);

        // TODO: Russian Roulette
    }

    return radiance;
}
