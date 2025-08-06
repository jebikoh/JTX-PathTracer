#pragma once

#include <jtx.hpp>

namespace jtx {

// -- Fujii Oren-Nayar --
static const float FON_AF_CONST    = 0.5f - (2.0f / 3.0f) * INV_PI;
static const float FON_E_AVG_CONST = (2.0f / 3.0f) - (28.0f / 15.0f * INV_PI);
static const auto FON_G_COEFFS     = vec4(0.0571085289, 0.491881867, -0.332181442, 0.0714429953);

JTX_FORCE_INLINE vec3 f_FON(const vec3 &wo, const vec3 &wi, const vec3 &rho, const float r) {
    const float Af = 1.0f / (1.0f + r * FON_AF_CONST);
    const float Bf = r * Af;
    const float s  = Dot(wo, wi) - (wi.z * wo.z);
    float invTf    = 1.0f;
    if (s > 0.0) {
        invTf = 1 / Max(Max(wi.z, wo.z), 1e-6f);
    }
    return rho * INV_PI * (Af + Bf * s * invTf);
}

// Average directional albedo
JTX_FORCE_INLINE float Eavg_FON(const float Af, const float Bf) {
    return Af + FON_E_AVG_CONST * Bf;
}

// Directional albedo
JTX_FORCE_INLINE float E_FON(const float cosTheta, const float Af, const float Bf) {
    // E_f(\omega_o) = \rho(A_F + (B_f / \pi) * G_F(\omega_o))
    //               = \rho(A_f + (B_f / \pi) * \pi * \sum_{k=1}^4 g_k(1 - \cos\theta_o)^k)
    //               = \rho(A_f + B_f * \sum_{k=1}^4 g_k(1 - \cos\theta_o)^k)
    const float x  = 1 - cosTheta;
    const float x2 = x * x;
    // I find this much more readable than using a 2x2 matrix...
    const float gOverPi = Dot(FON_G_COEFFS, vec4(x, x2, x * x2, x2 * x2));
    return Af + Bf * gOverPi;
}

// -- Energy-preserving Oren-Nayar --
static const float EON_EPS = 1e-7;

JTX_FORCE_INLINE vec3 f_EON(const vec3 &wo, const vec3 &wi, const vec3 &rho, const float r) {
    // SS (same as f_FON)
    const float Af = 1.0f / (1.0f + r * FON_AF_CONST);
    const float Bf = r * Af;
    const float s = Dot(wo, wi) - (wi.z * wo.z);
    float invTf = 1.0f;
    if (s > 0.0f) {
        invTf = 1.0f / Max(wi.z, wo.z);
    }
    const vec3 ss = rho * INV_PI * Af * (1.0f + r * s * invTf);

    // MS
    const float Ewo    = E_FON(wo.z, Af, Bf);
    const float Ewi    = E_FON(wi.z, Af, Bf);
    const float Eavg         = Eavg_FON(Af, Bf);
    const float oneMinusEavg = 1.0f - Eavg;

    const vec3 numerator   = rho * rho * Eavg * Max(EON_EPS, (1.0f - Ewo)) * Max(EON_EPS, (1.0f - Ewi));
    const vec3 denominator = (1.0f - rho * oneMinusEavg) * Max(EON_EPS, oneMinusEavg);
    const vec3 ms = numerator / denominator * INV_PI;

    return ss + ms;
}


class OrenNayarBRDF {
public:
    OrenNayarBRDF(const vec3 &reflectance, const float roughness) : m_rho(reflectance), m_r(roughness) {};

    vec3 Evaluate(const vec3& wo, const vec3& wi) const {
        if (!SameHemisphere(wo, wi)) return {};
        return m_rho * INV_PI;
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        vec3 wi = SampleCosineHemisphere(s1);
        if (wo.z < 0) { wi.z *= -1; }
        s.pdf = CosineHemispherePDF(AbsCosTheta(wi));
        s.f = f_EON(wo, wi, m_rho, m_r);
        s.wi = wi;
        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (!SameHemisphere(wo, wi)) return 0;
        return CosineHemispherePDF(AbsCosTheta(wi));
    }
private:
    vec3 m_rho;
    float m_r;
};

}// namespace jtx