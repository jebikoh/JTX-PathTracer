#pragma once

#include "bxdf.hpp"
#include "microfacet.hpp"

// Conductor Presets (wavelength-dependent IOR/k are not intuitive to set)
// https://chris.hindefjord.se/resources/rgb-ior-metals/
static const Material::Parameters JTX_BXDF_PRESET_ALUMINUM = {
    .ior = Vec3(1.34560, 0.96521, 0.61722),
    .k = Vec3(7.47460, 6.39950, 5.30310)
};

static const Material::Parameters JTX_BXDF_PRESET_BRASS = {
    .ior = Vec3(0.44400, 0.52700, 1.09400),
    .k = Vec3(3.69500, 2.76500, 1.82900)
};

static const Material::Parameters JTX_BXDF_PRESET_COPPER = {
    .ior = Vec3(0.27105, 0.67693, 1.31640),
    .k = Vec3(3.60920, 2.62480, 2.29210)
};

static const Material::Parameters JTX_BXDF_PRESET_GOLD = {
    .ior = Vec3(0.18299,0.42108,1.37340),
    .k = Vec3(3.42420,2.34590,1.77040)
};

static const Material::Parameters JTX_BXDF_PRESET_IRON = {
    .ior = Vec3(2.91140,2.94970,2.58450),
    .k = Vec3(3.08930,2.93180,2.76700)
};

static const Material::Parameters JTX_BXDF_PRESET_LEAD = {
    .ior = Vec3(1.91000,1.83000,1.44000),
    .k = Vec3(3.51000,3.40000,3.18000)
};

static const Material::Parameters JTX_BXDF_PRESET_MERCURY = {
    .ior = Vec3(2.07330,1.55230,1.06060),
    .k = Vec3(5.33830,4.65100,3.86280)
};

static const Material::Parameters JTX_BXDF_PRESET_PLATINUM = {
    .ior = Vec3(2.37570,2.08470,1.84530),
    .k = Vec3(4.26550,3.71530,3.13650)
};

static const Material::Parameters JTX_BXDF_PRESET_SILVER = {
    .ior = Vec3(0.15943,0.14512,0.13547),
    .k = Vec3(3.92910,3.19000,2.38080)
};

static const Material::Parameters JTX_BXDF_PRESET_TITANIUM = {
    .ior = Vec3(2.74070,2.54180,2.26700),
    .k = Vec3(3.81430,3.43450,3.03850)
};

static const Material::Parameters JTX_BXDF_PRESET_STAINLESS_STEEL = {
    .ior = Vec3(2.44010,2.04400,1.67780),
    .k = Vec3(4.18200,3.67320,3.11700)
};

class ConductorBxDF {
public:
    explicit ConductorBxDF(const GGX &mf, const Vec3 &eta, const Vec3 &k)
        : mf_(mf),
          eta_(eta),
          k_(k) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        if (mf_.smooth()) return {};

        // Torrance-Sparrow BRDF
        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);

        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        Vec3 w_m = w_i + w_o;
        if (w_m.lenSqr() == 0) return {};
        w_m = w_m.normalize();

        const Vec3 F = fresnelComplexRGB(jtx::absdot(w_o, w_m), eta_, k_);

        return mf_.D(w_m) * F * mf_.G(w_o, w_i) / (4 * cosTheta_i * cosTheta_o);
    }

    bool sample(const Vec3 &w_o, float uc, const Vec2f &u, BSDFSample &s) const {
        if (mf_.smooth()) {
            // For perfectly specular surfaces, we can just negate the x and y components
            const Vec3 w_i(-w_o.x, -w_o.y, w_o.z);
            const auto cosTheta_i = jtx::absCosTheta(w_i);
            const auto f          = fresnelComplexRGB(cosTheta_i, eta_, k_) / cosTheta_i;
            s                     = {f, w_i, 1, true};
            return true;
        }

        if (w_o.z == 0) return false;

        const Vec3 w_m = mf_.sampleWm(w_o, u);
        const Vec3 w_i = reflect(w_o, w_m);

        if (!jtx::sameHemisphere(w_o, w_i)) return false;

        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);
        if (cosTheta_o == 0 || cosTheta_i == 0) return false;

        const float _pdf = mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m));

        const Vec3 F = fresnelComplexRGB(jtx::absdot(w_o, w_m), eta_, k_);
        const Vec3 f = mf_.D(w_m) * F * mf_.G(w_o, w_i) / (4 * cosTheta_i * cosTheta_o);

        s = {f, w_i, _pdf, false};
        return true;
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        if (mf_.smooth()) return 0;
        if (!jtx::sameHemisphere(w_o, w_i)) return 0;
        Vec3 w_m = w_o + w_i;
        if (w_m.lenSqr() == 0) return 0;
        w_m = jtx::faceForward(w_m.normalize(), {0, 0, 1});
        return mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m));
    }

private:
    GGX mf_;
    Vec3 eta_;
    Vec3 k_;
};
