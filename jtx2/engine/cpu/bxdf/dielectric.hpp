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
        if (m_eta == 1 || m_ggx.IsSpecular()) return {};

        // TODO: rough conductor
        return {};
    }

    bool Sample(const vec3 &wo, const float s0, const vec2 &s1, BxDFSample &s) const {
        if (m_eta == 1 || m_ggx.IsSpecular()) {
            // Perfectly specular specular
            const float R = Fresnel(CosTheta(wo), m_eta);
            const float T = 1.0f - R;

            // If we do BDPT, add eta scaling term to account for non-symmetry
            if (s0 < R) {
                // Reflection (BRDF)
                s.wi = vec3(-wo.x, -wo.y, wo.z);
                s.f = vec3(R / AbsCosTheta(s.wi));
                s.pdf = R;
            } else {
                // Transmission (BTDF)
                s.eta                 = m_eta;
                const bool bRefracted = Refract(wo, vec3(0.0f, 0.0f, 1.0f), s.eta, s.wi);
                if (!bRefracted) return false;
                s.f = vec3(T) / AbsCosTheta(s.wi);
                s.pdf = T;
            }
            s.bSpecular = true;

            return true;
        }

        // TODO: rough specular
        return false;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (m_eta == 1 || m_ggx.IsSpecular()) return 0.0f;

        // TODO: rough specular
        return 1.0f;
    }

private:
    GGX m_ggx;
    float m_eta;// Relative index of refraction (eta_t / eta_i)
};

}// namespace jtx