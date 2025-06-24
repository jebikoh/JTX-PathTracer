#pragma once

#include <jtx.hpp>
#include <util/sampling.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>

namespace jtx {

class DiffuseBRDF {
public:
    explicit DiffuseBRDF(const vec3 &reflectance)
        : m_R(reflectance) {}

    vec3 Evaluate(const vec3& wo, const vec3& wi) const {
        if (!SameHemisphere(wo, wi)) return {};
        return m_R * INV_PI;
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        vec3 wi = SampleCosineHemisphere(s1);
        if (wo.z < 0) { wi.z = -1; }
        s.pdf = CosineHemispherePDF(AbsCosTheta(wi));
        s.f = m_R * INV_PI;
        s.wi = wi;
        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        if (!SameHemisphere(wo, wi)) return 0;
        return CosineHemispherePDF(AbsCosTheta(wi));
    }
private:
    vec3 m_R;
};

};
