#pragma once

#include <engine/cpu/bxdf/bxdf.hpp>
#include <engine/cpu/bxdf/microfacet.hpp>

namespace jtx {

class DielectricBxDF {
public:
    DielectricBxDF(const vec2 &alpha, const float eta)
        : m_ggx(alpha),
          m_eta(eta) {}

    // Constructor for perfectly specular dielectric
    explicit DielectricBxDF(const float eta)
        : m_ggx(vec2(0.0f, 0.0f)),
          m_eta(eta) {}

    vec3 Evaluate(const vec3 &wo, const vec3 &wi) const {
        // If the surface is perfectly specular, it is considered a dirac delta function
        if (m_eta == 1 || m_ggx.IsSmooth()) return {};

        // Compute half vector based off Snell's law
        const float cosThetaO = CosTheta(wo);
        const float cosThetaI = CosTheta(wi);
        const bool bReflected = cosThetaI * cosThetaO > 0.0f;

        float eta = 1.0f;
        if (!bReflected) eta = cosThetaO > 0 ? m_eta : 1 / m_eta;

        // This works because in the case of reflection: wm = wi + wo
        vec3 wm              = wi * eta + wo;
        const float wmLenSqr = wm.LengthSquared();
        // Avoid perfectly grazing angles (and avoid division by 0)
        if (cosThetaO == 0.0f || cosThetaI == 0.0f || wmLenSqr == 0.0f) return {};
        wm = FaceForward(wm / Sqrt(wmLenSqr), vec3(0, 0, 1));

        // Backfacing microfacets
        if (Dot(wm, wi) * cosThetaI < 0 || Dot(wm, wo) * cosThetaO < 0) return {};

        const float R = Fresnel(Dot(wo, wm), m_eta);
        const float T = 1 - R;

        // Reflectance
        if (bReflected) {
            const auto D = m_ggx.EvaluateNDF(wm);
            const auto G = m_ggx.EvaluateShadowingMasking(wo, wi);
            return vec3(D * G * R / Abs(4 * cosThetaI * cosThetaO));
        }

        // Transmission
        const float denominator = Sqr(Dot(wi, wm) + Dot(wo, wm) / eta) * Abs(cosThetaI * cosThetaO);
        if (denominator < 1e-7f) return {};

        const float numerator = m_ggx.EvaluateNDF(wm)  * T * m_ggx.EvaluateShadowingMasking(wo, wi) * Abs(Dot(wi, wm) * Dot(wo, wm));
        return vec3(numerator / denominator);
    }

    bool Sample(const vec3 &wo, const float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_eta == 1 || m_ggx.IsSmooth()) {
            // Perfectly specular specular
            const float R = Fresnel(CosTheta(wo), m_eta);
            const float T = 1.0f - R;

            // If we do BDPT, add eta scaling term to account for non-symmetry
            if (s0 < R) {
                // Reflection (BRDF)
                s.wi  = vec3(-wo.x, -wo.y, wo.z);
                s.f   = vec3(R / AbsCosTheta(s.wi));
                s.pdf = R;
            } else {
                // Transmission (BTDF)
                s.eta                 = m_eta;
                const bool bRefracted = Refract(wo, vec3(0.0f, 0.0f, 1.0f), s.eta, s.wi);
                if (!bRefracted) return false;
                s.f             = vec3(T) / AbsCosTheta(s.wi) / (s.eta * s.eta);
                s.pdf           = T;
                s.bTransmission = true;
            }
            s.bSpecular = true;

            return true;
        }

        const vec3 wm = m_ggx.SampleWm(wo, s1);

        const float R = Fresnel(Dot(wo, wm), m_eta);
        const float T = 1 - R;

        if (s0 < R) {
            // Reflection (BRDF)
            s.wi = Reflect(wo, wm);
            if (!SameHemisphere(wo, s.wi)) return false;

            const float cosThetaO = AbsCosTheta(wo);
            const float cosThetaI = AbsCosTheta(s.wi);
            if (cosThetaO == 0 || cosThetaI == 0) return false;

            s.pdf = m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm)) * R;

            const float D = m_ggx.EvaluateNDF(wm);
            const float G = m_ggx.EvaluateShadowingMasking(wo, s.wi);
            s.f = vec3(D * G * R / (4 * cosThetaO * cosThetaI));

            s.bTransmission = false;
        } else {
            // Transmission (BTDF)
            s.eta = m_eta;
            const bool bRefracted = Refract(wo, wm, s.eta, s.wi);
            if (SameHemisphere(wo, s.wi) || s.wi.z == 0 || !bRefracted) return false;

            float denominator = Sqr(Dot(s.wi, wm) + Dot(wo, wm) / s.eta);
            float numerator = m_ggx.PDF(wo, wm) * AbsDot(s.wi, wm);
            s.pdf =  numerator / denominator * T;

            const float D = m_ggx.EvaluateNDF(wm);
            const float G = m_ggx.EvaluateShadowingMasking(wo, s.wi);

            numerator = D * T * G * Abs(Dot(s.wi, wm) * Dot(wo, wm));
            denominator *= Abs(CosTheta(s.wi) * CosTheta(wo));
            if (denominator < 1e-7f) return false;

            s.f = vec3(numerator / denominator);
            s.bTransmission = true;
        }
        s.bSpecular = false;
        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_eta == 1 || m_ggx.IsSmooth()) return 0.0f;

        // Compute half vector based off Snell's law
        const float cosThetaO = CosTheta(wo);
        const float cosThetaI = CosTheta(wi);
        const bool bReflected = cosThetaI * cosThetaO > 0.0f;

        float eta = 1.0f;
        if (!bReflected) eta = cosThetaO > 0 ? m_eta : 1 / m_eta;

        // This works because in the case of reflection: wm = wi + wo
        vec3 wm              = wi * eta + wo;
        const float wmLenSqr = wm.LengthSquared();
        // Avoid perfectly grazing angles (and avoid division by 0)
        if (cosThetaO == 0.0f || cosThetaI == 0.0f || wmLenSqr == 0.0f) return 0.0f;
        wm = FaceForward(wm / Sqrt(wmLenSqr), vec3(0, 0, 1)); //

        // Backfacing microfacets
        if (Dot(wm, wi) * cosThetaI < 0 || Dot(wm, wo) * cosThetaO < 0) return 0.0f;

        const float R = Fresnel(Dot(wo, wm), m_eta);
        const float T = 1 - R;

        // Reflectance
        if (bReflected) {
            return m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm)) * R;
        }

        // Transmission
        const float denominator = Sqr(Dot(wi, wm) + Dot(wo, wm) / eta);
        const float numerator = m_ggx.PDF(wo, wm) * AbsDot(wi, wm);
        return numerator / denominator * T;
    }

private:
    GGX m_ggx;
    float m_eta;// Relative index of refraction (eta_t / eta_i)
};

}// namespace jtx