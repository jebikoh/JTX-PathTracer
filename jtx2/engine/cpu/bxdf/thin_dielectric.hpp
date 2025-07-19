#pragma once

#include <engine/cpu/bxdf/bxdf.hpp>
#include <engine/cpu/bxdf/microfacet.hpp>


namespace jtx {

class ThinDielectricBxDF {
public:

    explicit ThinDielectricBxDF(const float eta) : m_eta(eta) {}

    vec3 Evaluate(const vec3 &wo, const vec3 &wi) const {
        return {};
    }

    bool Sample(const vec3 &wo, const float s0, const vec2 &s1, BxDFSample &s) const {
        // Perfectly specular
        float R = Fresnel(CosTheta(wo), m_eta);
        float T = 1.0f - R;

        // Adjust for thin-dielectric interface
        if (R < 1) {
            R += (T * T * R) / (1 - (R * R));
            T = 1 - R;
        }

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
            s.f             = vec3(T) / AbsCosTheta(s.wi) / (s.eta * s.eta); // Non-symmetry
            s.pdf           = T;
            s.bTransmission = true;
        }
        s.bSpecular = true;

        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        return 0.0f;
    }

private:
    float m_eta;// Relative index of refraction (eta_t / eta_i)
};

}