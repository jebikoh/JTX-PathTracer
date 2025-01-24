#pragma once

#include "bxdf.hpp"

class DielectricBxDF {
public:
    explicit DielectricBxDF(const float eta) : eta(eta) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        return {};
    }

    BSDFSample sample(const Vec3 &w_o, float uc, const Vec2f &u) const {
        // Only handling perfectly specular case for now

        // Calculate Fresnel reflectance and complementary transmission
        const float R = fresnelDielectric(jtx::cosTheta(w_o), eta);
        const float T = 1 - R;

        // Pick between reflectance and transmission
        const float p = R / (R + T);
        if (uc < p) {
            // Reflectance: sample BRDF
            const Vec3 w_i = {-w_o.x, -w_o.y, w_o.z};
            const Vec3 f{R / jtx::absCosTheta(w_i)};

            return {f, w_i, p};
        } else {
            // Transmission: sample BTDF
            Vec3 w_i;
            float etap;

            // Technically, this should always refract
            // Due to FP errors, we need to check anyway
            const bool valid = refract(w_o, {0, 0, 1}, eta, &etap, w_i);
            if (!valid) return {};

            const Vec3 f = Vec3(T / jtx::absCosTheta(w_i)) / (etap * etap);
            return {f, w_i, 1 - p};
        }
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        return 0;
    }
private:
    float eta;
};
