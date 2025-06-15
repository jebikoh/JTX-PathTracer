#include <bvh/isect.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>
#include <engine/cpu/bxdf/diffuse.hpp>

namespace jtx {

// TODO: move frame to caller site

bool SampleBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) {
    const Frame frame  = Frame::FromZ(surface.normal);
    const vec3 woLocal = frame.ToLocal(wo);
    if (woLocal.z == 0.0f) return false;

    switch (surface.material->mType) {
        case Material::DIFFUSE: {
            // Skip texture for now
            const auto bxdf = DiffuseBRDF(surface.material->parameters.diffuse);
            bxdf.Sample(woLocal, s0, s1, s);
            if (s.pdf == 0.0f) return false;
            s.wi = frame.ToWorld(s.wi);
            return true;
        }
        default:
            return false;
    }
}

vec3 EvalBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, const vec3 &wi) {
    const Frame frame  = Frame::FromZ(surface.normal);
    const vec3 woLocal = frame.ToLocal(wo);
    const vec3 wiLocal = frame.ToLocal(wi);
    if (wiLocal.z == 0.0f || woLocal.z == 0.0f) {};

    switch (surface.material->mType) {
        case Material::DIFFUSE: {
            // Skip texture for now
            const auto bxdf = DiffuseBRDF(surface.material->parameters.diffuse);
            return bxdf.Evaluate(woLocal, wiLocal);
        }
        default:
            return {};
            break;
    }
}

float PDFBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, const vec3 &wi) {
    const Frame frame  = Frame::FromZ(surface.normal);
    const vec3 woLocal = frame.ToLocal(wo);
    const vec3 wiLocal = frame.ToLocal(wi);
    if (wiLocal.z == 0.0f || woLocal.z == 0.0f) return 0.0f;

    switch (surface.material->mType) {
        case Material::DIFFUSE: {
            // Skip texture for now
            const auto bxdf = DiffuseBRDF(surface.material->parameters.diffuse);
            return bxdf.PDF(woLocal, wiLocal);
        }
        default:
            return 0.0f;
            break;
    }
}

}