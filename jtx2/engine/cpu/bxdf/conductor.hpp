#pragma once

#include <engine/cpu/bxdf/microfacet.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>
#include <jtx.hpp>

namespace jtx {

class ComplexConductorBxDF {
public:
    ComplexConductorBxDF(const vec2 &alpha, const vec3 &eta, const vec3 &k)
        : m_ggx(alpha), m_eta(eta), m_k(k) {}

    // Constructor for perfectly specular conductor
    ComplexConductorBxDF(const vec3 &eta, const vec3 &k) :
        m_ggx(vec2(0.0f, 0.0f)), m_eta(eta), m_k(k) {}

    vec3 Evaluate(const vec3& wo, const vec3& wi) const {
        // If the surface is perfectly specular, it is considered a dirac delta function
        if (m_ggx.IsSpecular()) return {};
        if (!SameHemisphere(wo, wi)) return {};

        // TODO: rough conductor
        return {};
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_ggx.IsSpecular()) {
            // Perfectly specular conductor
            s.wi = vec3(-wo.x, -wo.y, wo.z);
            const float absCosThetaI = AbsCosTheta(s.wi);
            s.f = FresnelComplexRGB(absCosThetaI, m_eta, m_k) / absCosThetaI;
            s.pdf = 1.0f; // Dirac delta function
            s.bSpecular = true;
            return true;
        }

        // TODO: rough conductor
        return false;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_ggx.IsSpecular()) return 0.0f;
        if (!SameHemisphere(wo, wi)) return 0.0f;

        // TODO: rough conductor
        return 1.0f;
    }

private:
    GGX m_ggx;
    vec3 m_eta; // Relative IOR for RGB
    vec3 m_k;   // Extinction coefficient for RGB
};


// ConductorBxDF is a simplified version that uses a single Fresnel reflectance value (f0)
class ConductorBxDF {
public:
    ConductorBxDF(const vec2 &m_alpha, const vec3 &f0)
        : m_ggx(m_alpha), m_f0(f0) {}

    // Constructor for perfectly specular conductor
    explicit ConductorBxDF(const vec3 &f0) :
        m_ggx(vec2(0.0f, 0.0f)), m_f0(f0) {}

    vec3 Evaluate(const vec3& wo, const vec3& wi) const {
        // If the surface is perfectly specular, it is considered a dirac delta function
        if (m_ggx.IsSpecular()) return {};
        if (!SameHemisphere(wo, wi)) return {};

        // TODO: rough conductor
        return {};
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_ggx.IsSpecular()) {
            // Perfectly specular conductor
            s.wi = vec3(-wo.x, -wo.y, wo.z);
            const float absCosThetaI = AbsCosTheta(s.wi);
            s.f = Schlick(wo, vec3(0.0f, 0.0f, 1.0f),  m_f0) / absCosThetaI;
            s.pdf = 1.0f; // Dirac delta function
            s.bSpecular = true;
            return true;
        }

        // TODO: rough conductor
        return false;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_ggx.IsSpecular()) return 0.0f;
        if (!SameHemisphere(wo, wi)) return 0.0f;

        // TODO: rough conductor
        return 1.0f;
    }

private:
    GGX m_ggx;
    vec3 m_f0; // Fresnel reflectance at normal incidence (f0)
};

}
