#pragma once

#include "../sampling.hpp"

class DiffuseBRDF {
public:
    explicit DiffuseBRDF(const Vec3 &R)
        : R_(R) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return {};
        return R_ * INV_PI;
    }

    [[nodiscard]] BSDFSample sample(const Vec3 &w_o, float uc, const Vec2f &u) const {
        Vec3 w_i = sampleCosineHemisphere(u);
        if (w_o.z < 0) { w_i.z *= -1; }
        const float pdf = cosineHemispherePDF(jtx::absCosTheta(w_i));
        return {R_ * INV_PI, w_i, pdf};
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return 0;
        return cosineHemispherePDF(jtx::absCosTheta(w_i));
    }

private:
    Vec3 R_;
};
