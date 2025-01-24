#pragma once

#include "bxdf.hpp"

class ConductorBxDF {
public:
    explicit ConductorBxDF(const Vec3 &eta, const Vec3 &k) : eta(eta), k(k) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        return {};
    }

    [[nodiscard]] BSDFSample sample(const Vec3 &w_o, float uc, const Vec2f &u) const {
        // For perfectly specular surfaces, we can just negate the x and y components
        const Vec3 w_i(-w_o.x, -w_o.y, w_o.z);
        const auto cosTheta_i = jtx::absCosTheta(w_i);
        const auto f = fresnelComplexRGB(cosTheta_i, eta, k) / cosTheta_i;
        return {f, w_i, 1};
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        return 0;
    }
private:
    Vec3 eta;
    Vec3 k;
};