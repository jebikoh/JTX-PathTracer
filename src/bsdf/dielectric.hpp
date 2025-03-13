#pragma once

#include "bxdf.hpp"

class DielectricBxDF {
public:
    explicit DielectricBxDF(const GGX &ggx, const float eta) : eta(eta), mf_(ggx) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &wo, const Vec3 &wi) const {
        if (eta == 1 || mf_.smooth()) return {};
        // Evaluate rough BSDF
        // Half-vector
        // Look at pdf() for more details
        const float cosTheta_o = jtx::absCosTheta(wo);
        const float cosTheta_i = jtx::absCosTheta(wi);
        const bool reflect = cosTheta_o * cosTheta_i > 0;

        float etap = 1;
        if (!reflect) {
            etap = cosTheta_o > 0 ? eta : 1 / eta;
        }

        Vec3 wm = wi * etap + wo;
        if (cosTheta_i == 0 || cosTheta_o == 0 || wm.lenSqr() == 0) return {};
        wm = jtx::faceForward(wm.normalize(), {0, 0, 1});

        // Discard back-facing microfacets
        // Look at pdf() for more details
        if (jtx::dot(wm, wi) * cosTheta_i < 0 || jtx::dot(wm, wo) * cosTheta_o < 0) return {};

        const float F = fresnelDielectric(jtx::dot(wo, wm), eta);

        if (reflect) {
            return Vec3(mf_.D(wm) * F * mf_.G(wo, wi) / jtx::abs(4 * cosTheta_i * cosTheta_o));
        } else {
            const float a = mf_.D(wm) * (1 - F) * mf_.G(wo, wi) * jtx::abs(wi.dot(wm) * wo.dot(wm));
            const float b = jtx::sqr(wi.dot(wm) + wo.dot(wm) / etap) * jtx::abs(cosTheta_i * cosTheta_o);
            return Vec3(a / b);
        }
    }

    bool sample(const Vec3 &wo, const float uc, const Vec2f &u, BSDFSample &s) const {
        const bool isSpecular = mf_.smooth();
        if (eta == 1 || isSpecular) {
            // Calculate Fresnel reflectance and complementary transmission
            const float R = fresnelDielectric(jtx::cosTheta(wo), eta);
            const float T = 1 - R;

            // Pick between reflectance and transmission
            const float p = R / (R + T);
            if (uc < p) {
                // Reflectance: sample BRDF
                const Vec3 w_i = {-wo.x, -wo.y, wo.z};
                const Vec3 f{R / jtx::absCosTheta(w_i)};

                s = {f, w_i, p, 0, isSpecular};
                return true;
            } else {
                // Transmission: sample BTDF
                Vec3 w_i;
                float etap;

                // Technically, this should always refract
                // Due to FP errors, we need to check anyway
                const bool valid = refract(wo, {0, 0, 1}, eta, &etap, w_i);
                if (!valid) return false;

                const auto f = Vec3(T / jtx::absCosTheta(w_i));
                s = {f, w_i, 1 - p, 0, isSpecular};
                return true;
            }
        }

        const Vec3 wm = mf_.sampleWm(wo, u);
        const float R = fresnelDielectric(wo.dot(wm), eta);
        const float T = 1 - R;

        // float pdf;
        const float p = R / (R + T);
        if (uc < p) {
            // Reflection
            const Vec3 w_i = reflect(wo, wm);
            if (!jtx::sameHemisphere(wo, w_i)) return false;

            const float pdf = mf_.pdf(wo, wm) / (4 * jtx::absdot(wo, wm)) * p;
            const auto f = mf_.D(wm) * mf_.G(wo, w_i) * R / (4 * jtx::absCosTheta(w_i) * jtx::absCosTheta(wo));

            s = {Vec3(f), w_i, pdf, 0, isSpecular};
            return true;
        } else {
            // Transmission
            float etap;
            Vec3 wi;
            const bool totalInternalRefraction = !refract(wo, wm, eta, &etap, wi);
            if (jtx::sameHemisphere(wo, wi) || wi.z == 0 || totalInternalRefraction) return false;

            // PDF
            const float d = jtx::absdot(wi, wm) / jtx::sqr(wi.dot(wm) + wo.dot(wm) / etap);
            const float pdf = mf_.pdf(wo, wm) * d * (1 - p);

            // BRDF
            auto f = mf_.D(wm) * T * mf_.G(wo, wi) * jtx::abs(wi.dot(wm) * wo.dot(wm));
            f /= jtx::sqr(wi.dot(wm) + wm.dot(wo) / etap) * jtx::abs(jtx::cosTheta(wi) * jtx::cosTheta(wo));

            s = {Vec3(f), wi, pdf, 0, isSpecular};
            return true;
        }
    }

    [[nodiscard]] float pdf(const Vec3 &wo, const Vec3 &wi) const {
        if (eta == 1 || mf_.smooth()) return 0;
        // Compute half-vector

        // $\cos\theta_o$ and $\cos\theta_i$ are the perpendicular components of the
        // incident and outgoing directions, respectively.
        const float cosTheta_o = jtx::absCosTheta(wo);
        const float cosTheta_i = jtx::absCosTheta(wi);
        // If $\cos\theta_i$ and $\cos\theta_o$ are on the same side of the surface
        // their product will be >0
        const bool reflect = cosTheta_o * cosTheta_i > 0;

        // We need to update eta depending on the type of ray interactions
        float etap = 1;
        // In the case of refraction, we need to update eta depending on the orientation
        // of the rays
        if (!reflect) {
            // If $\cos\theta_o$ is > 0, the outgoing ray is on the same side as the normal
            // in which case the eta remains the same
            // If < 0, the outgoing ray is on the opposite side of the normal, so we flip eta
            etap = cosTheta_o > 0 ? eta : 1 / eta;
        }

        Vec3 wm = wi * etap + wo;
        if (cosTheta_i == 0 || cosTheta_o == 0 || wm.lenSqr() == 0) return 0;
        wm = jtx::faceForward(wm.normalize(), {0, 0, 1});

        // Discard back-facing microfacets
        // If the dot product is negative, they are back facing
        // However, we also need to respect the side of the surface. If $\omega_i$ or $\omega_o$ are on
        // the opposite side of the surface, we will need to negate the dot product.
        // We can do this by multiplying by their perpendicular component $\cos\theta$
        if (jtx::dot(wm, wi) * cosTheta_i < 0 || jtx::dot(wm, wo) * cosTheta_o < 0) return 0;

        // Determine fresnel reflectance and transmission
        const float R = fresnelDielectric(jtx::dot(wo, wm), eta);
        const float T = 1 - R;

        // Evaluate PDF
        float pdf;
        if (reflect) {
            pdf = mf_.pdf(wo, wm) / (4 * jtx::absdot(wo, wm)) * (R / (R + T));
        } else {
            const float d = jtx::absdot(wi, wm) / jtx::sqr(wi.dot(wm) + wo.dot(wm) / etap);
            pdf = mf_.pdf(wo, wm) * d * (T / (R + T));
        }
        return pdf;
    }
private:
    float eta;
    GGX mf_;
};

class ThinDielectricBxDF {
public:
    explicit ThinDielectricBxDF(const float eta) : eta(eta) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        return {};
    }

    bool sample(const Vec3 &w_o, float uc, const Vec2f &u, BSDFSample &s) const {
        // Only handling perfectly specular case for now

        // Calculate Fresnel reflectance and complementary transmission
        float R = fresnelDielectric(jtx::cosTheta(w_o), eta);
        float T = 1 - R;

        if (R < 1) {
            R += (T * T * R) / (1 - R * R);
            T = 1 - R;
        }

        // Pick between reflectance and transmission
        const float p = R / (R + T);
        if (uc < p) {
            // Reflectance: sample BRDF
            const Vec3 w_i = {-w_o.x, -w_o.y, w_o.z};
            const Vec3 f{R / jtx::absCosTheta(w_i)};

            s = {f, w_i, p};
            return true;
        } else {
            // Transmission: sample BTDF
            const Vec3 w_i = -w_o;
            const Vec3 f{T / jtx::absCosTheta(w_i)};
            s = {f, w_i, 1 - p};
            return true;
        }
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        return 0;
    }
private:
    float eta;

};
