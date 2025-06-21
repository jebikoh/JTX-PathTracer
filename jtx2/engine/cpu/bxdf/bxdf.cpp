#include "conductor.hpp"
#include "dielectric.hpp"


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
            if (bxdf.Sample(woLocal, s0, s1, s)) {
                if (s.pdf == 0.0f) return false;
                s.wi = frame.ToWorld(s.wi);
                return true;
            }
            return false;
        }
        case Material::CONDUCTOR: {
            const auto bxdf = ConductorBxDF(surface.material->parameters.f0);
            if (bxdf.Sample(woLocal, s0, s1, s)) {
                if (s.pdf == 0.0f) return false;
                s.wi = frame.ToWorld(s.wi);
                return true;
            }
            return false;
        }
        case Material::COMPLEX_CONDUCTOR: {
            const auto bxdf = ComplexConductorBxDF(surface.material->parameters.ior, surface.material->parameters.k);
            if (bxdf.Sample(woLocal, s0, s1, s)) {
                if (s.pdf == 0.0f) return false;
                s.wi = frame.ToWorld(s.wi);
                return true;
            }
            return false;
        }
        case Material::DIELECTRIC: {
            const auto bxdf = DielectricBxDF(surface.material->parameters.ior.x);
            if (bxdf.Sample(woLocal, s0, s1, s)) {
                if (s.pdf == 0.0f) return false;
                s.wi = frame.ToWorld(s.wi);
                return true;
            }
            return false;
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
        case Material::CONDUCTOR: {
            const auto bxdf = ConductorBxDF(surface.material->parameters.f0);
            return bxdf.Evaluate(woLocal, wiLocal);
        }
        case Material::COMPLEX_CONDUCTOR: {
            const auto bxdf = ComplexConductorBxDF(surface.material->parameters.ior, surface.material->parameters.k);
            return bxdf.Evaluate(woLocal, wiLocal);
        }
        case Material::DIELECTRIC: {
            const auto bxdf = DielectricBxDF(surface.material->parameters.ior.x);
            return bxdf.Evaluate(woLocal, wiLocal);
        }
        default:
            return {};
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
        case Material::CONDUCTOR: {
            const auto bxdf = ConductorBxDF(surface.material->parameters.f0);
            return bxdf.PDF(woLocal, wiLocal);
        }
        case Material::COMPLEX_CONDUCTOR: {
            const auto bxdf = ComplexConductorBxDF(surface.material->parameters.ior, surface.material->parameters.k);
            return bxdf.PDF(woLocal, wiLocal);
        }
        case Material::DIELECTRIC: {
            const auto bxdf = DielectricBxDF(surface.material->parameters.ior.x);
            return bxdf.PDF(woLocal, wiLocal);
        }
        default:
            return 0.0f;
    }
}

}