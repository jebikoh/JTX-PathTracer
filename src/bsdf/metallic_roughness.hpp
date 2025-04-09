#pragma once

#include "bxdf.hpp"
#include "microfacet.hpp"
#include "diffuse.hpp"

/**
 * glTF 2.0 Metallic-Roughness
 *
 * The model consists of dielectric and metal BRDFs mixed on the "metallic" parameter, both
 * using an isotropic Trowbridge-Reitz (GGX) microfacet distribution.
 *
 * The metallic BRDF combines wavelength-dependent IOR and extinction values (k) into a single
 * user defined reflectance at normal incidence, referred to as f0. The fresnel term is approximated
 * via Schlick
 *
 * The dielectric BRDF is a fresnel-weighted combination of a specular BRDF and a diffuse BRDF with a fixed
 * IOR of 1.5 (glass)
 *
 * The full calculation can be simplified (check spec), but we implement the full thing for reference
 */
class MetallicRoughnessBxDF {
public:
    explicit MetallicRoughnessBxDF(const float roughness2, const Vec3 &albedo, const float metallic) : mf_{roughness2, roughness2}, albedo_(albedo), metallic_(metallic) {}

    [[nodiscard]]
    Vec3 evaluate(const Vec3 &wo, const Vec3 &wi) const {
        const float cosTheta_o = jtx::absCosTheta(wo);
        const float cosTheta_i = jtx::absCosTheta(wi);
        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        return {};
    }

    bool sample(const Vec3 &wo, float uc, const Vec2f &u, BSDFSample &s) const {
        return false;
    }

    [[nodiscard]] float pdf(const Vec3 &wo, const Vec3 &wi) const {
        return false;
    }

private:
    GGX mf_;
    Vec3 albedo_;
    float metallic_;
};
