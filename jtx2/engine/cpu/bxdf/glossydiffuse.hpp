#pragma once
#include <jtx.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>

namespace jtx {

constexpr float INV_PI_23 = 1 / (23 * JTX_PI_F);

inline float Pow5(const float x) {
    const float x2 = x * x;
    return x2 * x2 * x;
}


// Implements: https://www.researchgate.net/publication/2523875_An_anisotropic_phong_BRDF_model
// Replaces the anisotropic phong distribution with GGX instead
// This is fairly unstable though -- will try some alternative instead
class GlossyDiffuseBxDF {
public:
    GlossyDiffuseBxDF(const vec2 &m_alpha, const vec3 &m_Rd, const vec3 &m_Rs)
    : m_Rd(m_Rd), m_Rs(m_Rs), m_ggx(m_alpha) {}

    vec3 Evaluate(const vec3 &wo, const vec3 &wi) const {
        vec3 wm = wo + wi;
        if (wm.x == 0 && wm.y == 0 && wm.z == 0) return {};
        wm = Normalize(wm);

        const vec3 fd  = EvaluateDiffuse(wo, wi);
        const vec3 fs = EvaluateSpecular(wo, wi, wm);

        return fd + fs;
    }

    bool Sample(const vec3 &wo, const float s0, const vec2 &s1, BxDFSample &s) const {
        // One-sample MIS
        if (s0 < 0.5) {
            // Sample wi via diffuse lobe
            s.wi = SampleCosineHemisphere(s1);
            if (wo.z < 0) s.wi.z *= -1;
        } else {
            // Sample wi via specular lobe
            const vec3 wm = m_ggx.SampleWm(wo, s1);
            s.wi = Reflect(wo, wm);
            if (!SameHemisphere(wo, s.wi)) return false;
        }
        s.f = Evaluate(wo, s.wi);
        s.pdf = PDF(wo, s.wi);

        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (!SameHemisphere(wo, wi)) return 0.0f;
        const vec3 wm = Normalize(wo + wi);

        const float ps = m_ggx.PDF(wo, wm) / (4 * Dot(wo, wm));
        const float pd = CosineHemispherePDF(AbsCosTheta(wi));
        return 0.5 * (ps + pd);
    }

private:
    vec3 Evaluate(const vec3 &wo, const vec3 &wi, const vec3 &wm) const {
        const vec3 fd  = EvaluateDiffuse(wo, wi);
        const vec3 fs = EvaluateSpecular(wo, wi, wm);

        return fd + fs;
    }

    vec3 EvaluateSpecular(const vec3 &wo, const vec3& wi, const vec3 &wm) const {
        const float D = m_ggx.EvaluateNDF(wm);
        const vec3  F = Schlick(wo, wm, m_Rs);
        return (D * F) / (4 * Dot(wo, wm) * std::max(AbsCosTheta(wi), AbsCosTheta(wo)));
    }

    vec3 EvaluateDiffuse(const vec3 &wo, const vec3 &wi) const {
        const float p1 = Pow5(1 - (0.5 * AbsCosTheta(wi)));
        const float p2 = Pow5(1 - (0.5 * AbsCosTheta(wo)));
        return 28 * INV_PI_23 * m_Rd * (1 - m_Rs) * (1 - p1) * (1 - p2);
    }

    vec3 m_Rd;
    vec3 m_Rs;
    GGX m_ggx;
};

}