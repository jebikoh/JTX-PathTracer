#pragma once

#include "bxdf.hpp"
#include "microfacet.hpp"

class MetallicRoughnessBxDF {
public:
    explicit MetallicRoughnessBxDF(const float roughness2, const Vec3 &albedo, const float metallic)
        : mf_{roughness2, roughness2},
          albedo(albedo),
          metallic(metallic) {}

    Vec3 evaluate(const Vec3 &wo, const Vec3 &wi) const {
        const float cosTheta_o = jtx::absCosTheta(wo);
        const float cosTheta_i = jtx::absCosTheta(wi);
        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        const Vec3 cDiff = jtx::lerp(albedo, Color::BLACK, metallic);
        const Vec3 f0    = jtx::lerp(Vec3(0.04f), albedo, metallic);

        Vec3 wm = wi + wo;
        if (wm.lenSqr() == 0) return {};
        wm = wm.normalize();

        const Vec3 F = schlick(wo, wm, f0);

        const Vec3 fDiffuse  = (1 - F) * cDiff * INV_PI;
        const Vec3 fSpecular = mf_.D(wm) * F * mf_.G(wo, wi) / (4 * jtx::absCosTheta(wi) * jtx::absCosTheta(wo));

        return fDiffuse + fSpecular;
    }

    bool sample(const Vec3 &wo, const float uc, const Vec2f &u, BSDFSample &s) const {
        // Mixture
        const float cosTheta_o = jtx::absCosTheta(wo);
        if (cosTheta_o == 0) return false;

        const Vec3 cDiff = jtx::lerp(albedo, Color::BLACK, metallic);
        const Vec3 f0    = jtx::lerp(Vec3(0.04f), albedo, metallic);
        Vec3 F     = schlick(wo, Vec3(0, 0, 1), f0);

        const float specularWeight = F.average();
        const float diffuseWeight  = (1 - metallic) * (1 - specularWeight); // Dielectric lobe -> Diffuse lobe
        const float totalWeight    = specularWeight + diffuseWeight;

        float p = 1.0f;
        if (totalWeight > 0) p = specularWeight / totalWeight;

        Vec3 wi;
        Vec3 wm;
        float pdf;

        if (uc < p) {
            // Specular via GGX
            if (wo.z == 0) return false;
            wm = mf_.sampleWm(wo, u);
            wi = jtx::reflect(wo, wm);

            if (!jtx::sameHemisphere(wo, wi)) return false;

            const float cosTheta_i = jtx::absCosTheta(wi);
            if (cosTheta_i == 0) return false;

            pdf = mf_.pdf(wo, wm) / (4 * jtx::absdot(wo, wm));
        } else {
            // Diffuse
            wi = sampleCosineHemisphere(u);
            if (wo.z < 0) { wi.z *= -1; }

            Vec3 w_m = wi + wo;
            if (w_m.lenSqr() == 0) return {};
            w_m = w_m.normalize();

            pdf = cosineHemispherePDF(jtx::absCosTheta(wi));
        }

        F = schlick(wo, wm, f0);

        const Vec3 fDiffuse  = (1 - F) * (cDiff / PI);
        const Vec3 fSpecular = mf_.D(wm) * F * mf_.G(wo, wi) / (4 * jtx::absCosTheta(wi) * jtx::absCosTheta(wo));

        s.pdf = pdf;
        s.w_i = wi;
        s.fSample = fDiffuse + fSpecular;

        return true;
    }

    float pdf(const Vec3 &wo, const Vec3 &wi) const {
        if (!jtx::sameHemisphere(wo, wi)) return 0;

        // Mixture
        const float cosTheta_o = jtx::absCosTheta(wo);
        if (cosTheta_o == 0) return false;

        const Vec3 f0    = jtx::lerp(Vec3(0.04f), albedo, metallic);
        const Vec3 F     = schlick(wo, Vec3(0, 0, 1), f0);

        const float specularWeight = F.average();
        const float diffuseWeight  = (1 - metallic) * (1 - specularWeight); // Dielectric lobe -> Diffuse lobe
        const float totalWeight    = specularWeight + diffuseWeight;

        float p = 1.0f;
        if (totalWeight > 0) p = specularWeight / totalWeight;

        Vec3 wm = wi + wo;
        if (wm.lenSqr() == 0) return 0.0f;
        wm = jtx::faceForward(wm.normalize(), {0, 0, 1});

        const float specularPdf = mf_.pdf(wo, wm) / (4 * jtx::absdot(wo, wm));
        const float diffusePdf  = cosineHemispherePDF(jtx::absCosTheta(wi));

        return p * specularPdf + (1 - p) * diffusePdf;
    }

private:
    GGX mf_;
    Vec3 albedo;
    float metallic;
};
