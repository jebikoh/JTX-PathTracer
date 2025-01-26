#pragma once

#include "bxdf.hpp"
#include "microfacet.hpp"

class ConductorBxDF {
public:
    explicit ConductorBxDF(const TrowbridgeReitz &mf, const Vec3 &eta, const Vec3 &k) : mf_(mf), eta_(eta), k_(k) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        if (mf_.smooth()) return {};

        // Torrance-Sparrow BRDF
        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);

        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        Vec3 w_m = w_i + w_o;
        if (w_m.lenSqr() == 0) return {};
        w_m = w_m.normalize();

        Vec3 F = fresnelComplexRGB(jtx::absdot(w_o, w_m), eta_, k_);

        return mf_.D(w_m) * F * mf_.G(w_o, w_i) / (4 * cosTheta_i * cosTheta_o);
    }

    [[nodiscard]] BSDFSample sample(const Vec3 &w_o, float uc, const Vec2f &u) const {
        if (mf_.smooth()) {
            // For perfectly specular surfaces, we can just negate the x and y components
            const Vec3 w_i(-w_o.x, -w_o.y, w_o.z);
            const auto cosTheta_i = jtx::absCosTheta(w_i);
            const auto f = fresnelComplexRGB(cosTheta_i, eta_, k_) / cosTheta_i;
            return {f, w_i, 1};
        }
        if (w_o.z == 0) return {};

        const Vec3 w_m = mf_.sampleW_m(w_o, u);
        const Vec3 w_i = reflect(w_o, w_m);

        if (!jtx::sameHemisphere(w_o, w_i)) return {};

        const float _pdf = mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m));

        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);
        if (cosTheta_o == 0 || cosTheta_i == 0) return {};

        const Vec3 F = fresnelComplexRGB(jtx::absdot(w_o, w_m), eta_, k_);
        const Vec3 f = mf_.D(w_m) * F * mf_.G(w_o, w_i) / (4 * cosTheta_i * cosTheta_o);

        return {f, w_i, _pdf};
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        if (mf_.smooth()) return 0;
        Vec3 w_m = w_o + w_i;
        if (w_m.lenSqr() == 0) return 0;
        w_m = jtx::faceForward(w_m.normalize(), {0,0, 1});
        return mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m));
    }

private:
    TrowbridgeReitz mf_;
    Vec3 eta_;
    Vec3 k_;
};