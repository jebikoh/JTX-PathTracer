#include <bvh/bvh.hpp>
#include <bvh/isect.hpp>
#include <engine/cpu/integrator.hpp>
#include <util/sampling.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>

// Basic path tracer without MIS or NEE
vec3 jtx::IntegrateBasic(ray r, const Scene &scene, const BVH &bvh, int maxDepth, Sampler &rng) {
    auto radiance = vec3(0.0f);
    auto beta = vec3(1.0f);
    int depth = 0;

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
