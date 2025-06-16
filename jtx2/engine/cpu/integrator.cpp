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

// Next event estimation (NEE) path tracer
// Does NOT apply MIS
vec3 jtx::IntegrateNEE(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
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

        if (depth++ == maxDepth) break;

        SurfaceAttributes surface;
        InterpolateVertexAttributes(scene, r, triIsect, surface);

        const vec3 wo = -r.dir;

        // Sample direct illumination
        // For now, we will only deal with emissive triangles
        {
            const auto index = rng.Sample(scene.emissiveTriangles.size());
            const auto emissiveIndex = scene.emissiveTriangles[index];


            LightSample lightSample;
            scene.SampleEmissiveTriangle(rng.Uniform<vec2>(), emissiveIndex, lightSample);

            // Check for visibility
            vec3 shadowRayOrigin = surface.point + surface.normal * 0.001f; // Offset to avoid self-shadowing
            vec3 shadowRayDir = normalize(lightSample.position - shadowRayOrigin);
            ray shadowRay(shadowRayOrigin, shadowRayDir);
            if (!bvh.AnyHit(shadowRay, 0.001f, JTX_INFINITY_F)) {
                // Ray hit the light source, accumulate direct illumination
                vec3 wi = shadowRayDir;

                // Evaluate BxDF
                const auto f = EvalBxDF(scene, surface, wo, wi);
                const float pdfBxDF = PDFBxDF(scene, surface, wo, wi);

                // P of selecting this light * P of selecting that point
                const float pdfLight = (1.0f / static_cast<float>(scene.emissiveTriangles.size()) * lightSample.pdf);

                // What now?
                // Also, what is up with the crazy solid angle -> area conversion that I see in the literature?

            }
        }

        // Sample indirect
        const float s = rng.Uniform<float>();
        const vec2 s2 = rng.Uniform<vec2>();

        BxDFSample sample;
        bool bSuccess = SampleBxDF(scene, surface, wo, s, s2, sample);
        if (!bSuccess) break;

        beta *= sample.f * AbsDot(sample.wi, surface.normal) / sample.pdf;
    }

    return radiance;
}
