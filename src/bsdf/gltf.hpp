#pragma once

#include "bxdf.hpp"
#include "diffuse.hpp"
#include "microfacet.hpp"

// Roughness passed in directly to GGX
class metallicRoughnessBxDF {
public:
    explicit metallicRoughnessBxDF(const float roughness2, const Vec3 &albedo, const float metallic)
        : mf_{roughness2, roughness2},
          albedo(albedo),
          metallic(metallic) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &wo, const Vec3 &wi) const {
        const float cosTheta_o = jtx::absCosTheta(wo);
        const float cosTheta_i = jtx::absCosTheta(wi);
        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        const Vec3 cDiff = jtx::lerp(albedo, Color::BLACK, metallic);
        const Vec3 f0    = jtx::lerp(Vec3(0.04f), albedo, metallic);

        Vec3 wm = wi + wo;
        if (wm.lenSqr() == 0) return {};
        wm = wm.normalize();

        const Vec3 F = schlick(wo, wm, f0);

        const Vec3 fDiffuse  = (1 - F) * (cDiff / PI);
        const Vec3 fSpecular = mf_.D(wm) * F * mf_.G(wo, wi) / (4 * jtx::absCosTheta(wi) * jtx::absCosTheta(wo));

        return fDiffuse + fSpecular;
    }

private:
    GGX mf_;
    Vec3 albedo;
    float metallic;
};
