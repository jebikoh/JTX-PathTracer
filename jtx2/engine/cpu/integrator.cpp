#include <bvh/bvh.hpp>
#include <bvh/isect.hpp>
#include <engine/cpu/integrator.hpp>
#include <util/sampling.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>

// Basic path tracer without MIS or NEE
vec3 jtx::Integrate(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    auto radiance = vec3(0.0f);
    auto beta     = vec3(1.0f);
    int depth     = 0;

    TriangleIntersection triIsect;
    while (beta) {
        const bool bHit = bvh.ClosestHit(r, 0.001f, JTX_INFINITY_F, triIsect);
        if (!bHit) {
            radiance += beta * scene.skyColor;
            break;
        }

        // Interpolate vertex attributes
        SurfaceAttributes surface;
        InterpolateVertexAttributes(scene, r, triIsect, surface);

        radiance += beta * surface.material->parameters.emission;

        if (depth++ == maxDepth) break;

        vec3 wo = -r.dir;

        const float s = rng.Uniform<float>();
        const vec2 s2 = rng.Uniform<vec2>();

        BxDFSample sample;
        bool bSuccess = SampleBxDF(scene, surface, wo, s, s2, sample);
        if (!bSuccess) break;

        beta *= sample.f * AbsDot(sample.wi, surface.normal) / sample.pdf;
        r = Ray(surface.point + sample.wi * 0.001f, sample.wi, triIsect.t);
    }

    return radiance;
}

vec3 jtx::IntegrateRR(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    auto radiance = vec3(0.0f);
    auto beta     = vec3(1.0f);
    int depth     = 0;

    TriangleIntersection triIsect;
    while (beta) {
        const bool bHit = bvh.ClosestHit(r, 0.001f, JTX_INFINITY_F, triIsect);
        if (!bHit) {
            radiance += beta * scene.skyColor;
            break;
        }

        // Interpolate vertex attributes
        SurfaceAttributes surface;
        InterpolateVertexAttributes(scene, r, triIsect, surface);

        radiance += beta * surface.material->parameters.emission;

        if (depth++ == maxDepth) break;

        vec3 wo = -r.dir;

        const float s = rng.Uniform<float>();
        const vec2 s2 = rng.Uniform<vec2>();

        BxDFSample sample;
        bool bSuccess = SampleBxDF(scene, surface, wo, s, s2, sample);
        if (!bSuccess) break;

        beta *= sample.f * AbsDot(sample.wi, surface.normal) / sample.pdf;

        // Russian roulette
        if (depth > JTX_RR_MIN_DEPTH) {
            const float p = std::min(1.0f, beta.MaxComponent());
            if (rng.Uniform<float>() >= p) break;
            beta /= p;
        }

        r = Ray(surface.point + sample.wi * 0.001f, sample.wi, triIsect.t);
    }

    return radiance;
}

// Path tracer with Next Event Estimation (NEE)
// Samples both direct lighting via light sampling and indirect lighting via BxDF sampling
// Does not apply multiple importance sampling (MIS) the two (0-1 weighting)
vec3 jtx::IntegrateNEE(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    auto radiance        = vec3(0.0f);
    auto beta            = vec3(1.0f);
    int depth            = 0;
    bool bSpecularBounce = false;

    TriangleIntersection triIsect;
    while (true) {
        const bool bHit = bvh.ClosestHit(r, 0.001f, JTX_INFINITY_F, triIsect);
        if (!bHit) {
            radiance += beta * scene.skyColor;
            break;
        }

        SurfaceAttributes surface;
        InterpolateVertexAttributes(scene, r, triIsect, surface);

        // Only add emission on the first bounce or if the previous bounce was specular
        if (depth == 0 || bSpecularBounce) {
            radiance += beta * surface.material->parameters.emission;
        }

        if (depth++ == maxDepth) break;

        const vec3 wo = -r.dir;

        // Sample direct illumination (NEE)
        //
        // If the previous bounce was specular, we skip direct lighting sampling since specular
        // surfaces are modelled with a dirac delta function
        auto Ld = vec3(0.0f);
        if (!bSpecularBounce) {
            // Randomly select a light
            const auto index         = rng.Sample(scene.emissiveTriangles.size());
            const auto emissiveIndex = scene.emissiveTriangles[index];

            // Sample a point on that light
            LightSample lightSample;
            scene.SampleTriangle(rng.Uniform<vec2>(), emissiveIndex, lightSample);

            // Calculate the incident light direction
            vec3 wi           = lightSample.position - surface.point;
            const float dist2 = wi.LengthSquared();
            const float dist  = jtx::SafeSqrt(dist2);
            wi /= dist;

            // Evaluate visibility term
            ray shadowRay(surface.point + surface.normal * 0.001f, wi);
            // For larger scenes, the delta might need to be adjusted to be bigger
            // When the vertex distances start exceeding the 100s, <=0.001f leads to artifacts
            if (!bvh.AnyHit(shadowRay, 0.0f, dist - 0.001f)) {
                // Ray was not obscured, V = 1.0f
                vec3 Li = lightSample.emission;

                // Incident angle cosine factors
                const float cosThetaS = AbsDot(wi, surface.normal);     // From surface
                const float cosThetaL = AbsDot(-wi, lightSample.normal);// From light

                // P of selecting this light * P of selecting that point
                const float pdfLight = (1.0f / static_cast<float>(scene.emissiveTriangles.size()) * lightSample.pdf);
                // Evaluate BxDF
                const auto f = EvalBxDF(scene, surface, wo, wi);

                // Compute the contribution of this light sample
                Ld = f * Li * cosThetaS * (cosThetaL / dist2) / pdfLight;
            }
        }
        // Add direct lighting contribution to radiance
        radiance += beta * Ld;

        // Sample indirect via BxDF sampling
        const float s = rng.Uniform<float>();
        const vec2 s2 = rng.Uniform<vec2>();

        BxDFSample sample;
        bool bSuccess = SampleBxDF(scene, surface, wo, s, s2, sample);
        if (!bSuccess) break;

        bSpecularBounce = sample.bSpecular;

        beta *= sample.f * AbsDot(sample.wi, surface.normal) / sample.pdf;

        // Russian roulette
        if (depth > JTX_RR_MIN_DEPTH) {
            const float p = std::min(1.0f, beta.MaxComponent());
            if (rng.Uniform<float>() >= p) break;
            beta /= p;
        }

        r = Ray(surface.point + sample.wi * 0.001f, sample.wi);
    }

    return radiance;
}

vec3 jtx::IntegrateMIS(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    auto radiance = vec3(0.0f);
    auto beta = vec3(1.0f);
    int depth = 0;
    bool bSpecularBounce = false;
    float probBRDF = 1.0f;

    TriangleIntersection triIsect;
    while (true) {
        const bool bHit = bvh.ClosestHit(r, 0.001f, JTX_INFINITY_F, triIsect);
        if (!bHit) {
            radiance += vec3(0.0f, 0.0f, 0.0f); // No contribution if no hit (sky is only black for now)
            break;
        }

        SurfaceAttributes surface;
        InterpolateVertexAttributes(scene, r, triIsect, surface);
        const auto &parameters = surface.material->parameters;

        // Incorporate emission from the surface if it has any
        if (parameters.emission) {
            if (depth == 0 || bSpecularBounce) {
                radiance += beta * parameters.emission;
            } else {
                // Since we hit a light source, we need to apply MIS to figure out how to weigh it
                //
                // We need to calculate the light weight, which is the product of
                // P(selecting this light) * P(sampling that point on the light)
                // Note, the scattering PDF, BSDF, and cosine factors already included in beta
                const float cosTheta = AbsDot(surface.normal, -r.dir);
                const float dist2 = (r.origin - surface.point).LengthSquared();
                float probLight = 1.0f / static_cast<float>(scene.emissiveTriangles.size()) * scene.TrianglePDF(triIsect.index) * dist2 / cosTheta;

                float wb = BalanceHeuristic(probBRDF, probLight);
                radiance += beta * parameters.emission * wb;
            }
        }

        if (depth++ == maxDepth) break;

        const vec3 wo = -r.dir;

        // Sample direct illumination (NEE)
        //
        // If the previous bounce was specular, we skip direct lighting sampling since specular
        // surfaces are modelled with a dirac delta function
        auto Ld = vec3(0.0f);
        if (!bSpecularBounce) {
            // Randomly select a light
            const auto index         = rng.Sample(scene.emissiveTriangles.size());
            const auto emissiveIndex = scene.emissiveTriangles[index];

            // Sample a point on that light
            LightSample lightSample;
            scene.SampleTriangle(rng.Uniform<vec2>(), emissiveIndex, lightSample);

            // Calculate the incident light direction
            vec3 wi           = lightSample.position - surface.point; // surface -> light
            const float dist2 = wi.LengthSquared();
            const float dist  = jtx::SafeSqrt(dist2);
            wi /= dist;

            // Evaluate visibility term
            // For larger scenes, the delta might need to be adjusted to be bigger
            // When the vertex distances start exceeding the 100s, <=0.001f leads to artifacts
            ray shadowRay(surface.point + surface.normal * 0.001f, wi);
            bool bOccluded = bvh.AnyHit(shadowRay, 0.0f, dist - 0.01f);
            if (!bOccluded) {
                // Ray was not obscured, V = 1.0f
                const auto f = EvalBxDF(scene, surface, wo, wi) * AbsDot(wi, surface.normal);
                if (f) {
                    vec3 Le = lightSample.emission;

                    // Incident angle cosine factor from surface
                    const float cosThetaL = AbsDot(-wi, lightSample.normal);

                    // P of selecting this light * P of selecting that point
                    // Note, P of selecting that point needs to be converted to solid angle
                    const float nProbLight_Area = 1.0f / static_cast<float>(scene.emissiveTriangles.size()) * lightSample.pdf;
                    const float nProbLight_SolidAngle = nProbLight_Area * dist2 / cosThetaL;
                    const float nProbBRDF = PDFBxDF(scene, surface, wo, wi);

                    const float wl = BalanceHeuristic(nProbLight_SolidAngle, nProbBRDF);

                    // Compute the contribution of this light sample
                    Ld = wl * f * Le / nProbLight_SolidAngle;
                }
            }
        }
        // Add direct lighting contribution to radiance
        radiance += beta * Ld;

        // Sample indirect via BxDF sampling
        const float s = rng.Uniform<float>();
        const vec2 s2 = rng.Uniform<vec2>();

        BxDFSample sample;
        bool bSuccess = SampleBxDF(scene, surface, wo, s, s2, sample);
        if (!bSuccess) break;

        bSpecularBounce = sample.bSpecular;
        probBRDF = sample.pdf;

        beta *= sample.f * AbsDot(sample.wi, surface.normal) / sample.pdf;

        // Russian roulette
        if (depth > JTX_RR_MIN_DEPTH) {
            const float p = std::min(1.0f, beta.MaxComponent());
            if (rng.Uniform<float>() >= p) break;
            beta /= p;
        }

        r = Ray(surface.point + sample.wi * 0.001f, sample.wi);
    }

    return radiance;
}
