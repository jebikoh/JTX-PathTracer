#pragma once

#include <engine/cpu/bxdf/microfacet.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>
#include <jtx.hpp>

namespace jtx {

inline vec3 FaceForward(vec3 v, const vec3 &n) {
    return Dot(v, n) < 0 ? -v : v;
}

class ComplexConductorBxDF {
public:
    ComplexConductorBxDF(const vec2 &alpha, const vec3 &eta, const vec3 &k)
        : m_ggx(alpha), m_eta(eta), m_k(k) {}

    // Constructor for perfectly specular conductor
    ComplexConductorBxDF(const vec3 &eta, const vec3 &k) :
        m_ggx(vec2(0.0f, 0.0f)), m_eta(eta), m_k(k) {}

    vec3 Evaluate(const vec3& wo, const vec3& wi) const {
        // If the surface is perfectly specular, it is considered a dirac delta function
        if (m_ggx.IsSmooth()) return {};
        if (!SameHemisphere(wo, wi)) return {};

        const float cosThetaO = AbsCosTheta(wo);
        const float cosThetaI = AbsCosTheta(wi);
        if (cosThetaO == 0 || cosThetaI == 0) return {};

        vec3 wm = wi + wo;
        if (wm.LengthSquared() == 0) return {};
        wm = Normalize(wm);

        const auto D = m_ggx.EvaluateNDF(wm);
        const auto F = FresnelComplexRGB(AbsDot(wo, wm), m_eta, m_k);
        const auto G = m_ggx.EvaluateShadowingMasking(wo, wi);

        return D * F * G / (4 * cosThetaO * cosThetaI);
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_ggx.IsSmooth()) {
            // Perfectly specular conductor
            s.wi                     = vec3(-wo.x, -wo.y, wo.z);
            const float absCosThetaI = AbsCosTheta(s.wi);
            s.f                      = FresnelComplexRGB(absCosThetaI, m_eta, m_k) / absCosThetaI;
            s.pdf                    = 1.0f;// Dirac delta function
            s.bSpecular              = true;
            return true;
        }
        if (wo.z == 0.0f) return false;

        const vec3 wm = m_ggx.SampleWm(wo, s1);
        s.wi = Reflect(wo, wm);
        if (!SameHemisphere(wo, s.wi)) return false;

        s.pdf = m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));

        const float absCosThetaO = AbsCosTheta(wo);
        const float absCosThetaI = AbsCosTheta(s.wi);
        if (absCosThetaO == 0 || absCosThetaI == 0) return false;

        const float D = m_ggx.EvaluateNDF(wm);
        const vec3 F = FresnelComplexRGB(AbsDot(wo, wm), m_eta, m_k);
        const float G = m_ggx.EvaluateShadowingMasking(wo, s.wi);

        s.f = D * F * G / (4 * absCosThetaI * absCosThetaO);
        s.bSpecular = false;

        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_ggx.IsSmooth()) return 0.0f;
        if (!SameHemisphere(wo, wi)) return 0.0f;

        vec3 wm = wo + wi;
        if (wm.LengthSquared() == 0) return 0.0f;
        wm = FaceForward(Normalize(wm), vec3(0.0f, 0.0f, 1.0f));
        // PDF of GGX times the Jacobian of the half-direction transformation
        return m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));
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
        if (m_ggx.IsSmooth()) return {};
        if (!SameHemisphere(wo, wi)) return {};

        const float cosThetaO = AbsCosTheta(wo);
        const float cosThetaI = AbsCosTheta(wi);
        if (cosThetaO == 0 || cosThetaI == 0) return {};

        vec3 wm = wi + wo;
        if (wm.LengthSquared() == 0) return {};
        wm = Normalize(wm);

        const auto D = m_ggx.EvaluateNDF(wm);
        const auto F = Schlick(wo, wm,  m_f0);
        const auto G = m_ggx.EvaluateShadowingMasking(wo, wi);

        return D * F * G / (4 * cosThetaO * cosThetaI);
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_ggx.IsSmooth()) {
            // Perfectly specular conductor
            s.wi                     = vec3(-wo.x, -wo.y, wo.z);
            const float absCosThetaI = AbsCosTheta(s.wi);
            s.f                      = Schlick(wo, vec3(0.0f, 0.0f, 1.0f),  m_f0) / absCosThetaI;
            s.pdf                    = 1.0f;// Dirac delta function
            s.bSpecular              = true;
            return true;
        }

        if (wo.z == 0.0f) return false;

        const vec3 wm = m_ggx.SampleWm(wo, s1);
        s.wi = Reflect(wo, wm);
        if (!SameHemisphere(wo, s.wi)) return false;

        s.pdf = m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));

        const float absCosThetaO = AbsCosTheta(wo);
        const float absCosThetaI = AbsCosTheta(s.wi);
        if (absCosThetaO == 0 || absCosThetaI == 0) return false;

        const float D = m_ggx.EvaluateNDF(wm);
        const vec3 F = Schlick(wo, wm,  m_f0);
        const float G = m_ggx.EvaluateShadowingMasking(wo, s.wi);

        s.f = D * F * G / (4 * absCosThetaI * absCosThetaO);
        s.bSpecular = false;

        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_ggx.IsSmooth()) return 0.0f;
        if (!SameHemisphere(wo, wi)) return 0.0f;

        vec3 wm = wo + wi;
        if (wm.LengthSquared() == 0) return 0.0f;
        wm = FaceForward(Normalize(wm), vec3(0.0f, 0.0f, 1.0f));
        // PDF of GGX times the Jacobian of the half-direction transformation
        return m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));
    }

private:
    GGX m_ggx;
    vec3 m_f0; // Fresnel reflectance at normal incidence (f0)
};

}
