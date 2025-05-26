#pragma once
#include <jtx.hpp>
#include <util/sampling.hpp>

namespace jtx {

class DiffuseBRDF {
public:
    explicit DiffuseBRDF(const vec3 &reflectance)
        : m_r(reflectance) {}

    vec3 evaluate(const vec3& wo, const vec3& wi) const {
        if (!jtx::sameHemisphere(wo, wi)) return {};
        return m_r * INV_PI;
    }

private:
    vec3 m_r;
};

};
