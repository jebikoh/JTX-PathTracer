#pragma once

#include "bxdf.hpp"

class DielectricBxDF {
public:
    explicit DielectricBxDF(const GGX &ggx, const float eta) : eta(eta), mf_(ggx) {}

    [[nodiscard]] Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        if (eta == 1 || mf_.smooth()) return {};
        // Evaluate rough BSDF
        // Half-vector
        // Look at pdf() for more details
        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);
        const bool reflect = cosTheta_o * cosTheta_i > 0;

        float etap = 1;
        if (!reflect) {
            etap = cosTheta_o > 0 ? eta : 1 / eta;
        }

        Vec3 w_m = w_i * etap + w_o;
        if (cosTheta_i == 0 || cosTheta_o == 0 || w_m.lenSqr() == 0) return {};
        w_m = jtx::faceForward(w_m.normalize(), {0, 0, 1});

        // Discard back-facing microfacets
        // Look at pdf() for more details
        if (jtx::dot(w_m, w_i) * cosTheta_i < 0 || jtx::dot(w_m, w_o) * cosTheta_o < 0) return {};

        const float F = fresnelDielectric(jtx::dot(w_o, w_m), eta);

        if (reflect) {
            return Vec3(mf_.D(w_m) * F * mf_.G(w_o, w_i) / jtx::abs(4 * cosTheta_i * cosTheta_o));
        } else {
            const float a = mf_.D(w_m) * (1 - F) * mf_.G(w_o, w_i) * jtx::abs(w_i.dot(w_m) * w_o.dot(w_m));
            const float b = jtx::sqr(w_i.dot(w_m) + w_o.dot(w_m) / etap) * jtx::abs(cosTheta_i * cosTheta_o);
            return Vec3(a / b);
        }
    }

    bool sample(const Vec3 &w_o, const float uc, const Vec2f &u, BSDFSample &s) const {
        bool isSpecular = mf_.smooth();
        if (eta == 1 || isSpecular) {
            // Calculate Fresnel reflectance and complementary transmission
            const float R = fresnelDielectric(jtx::cosTheta(w_o), eta);
            const float T = 1 - R;

            // Pick between reflectance and transmission
            const float p = R / (R + T);
            if (uc < p) {
                // Reflectance: sample BRDF
                const Vec3 w_i = {-w_o.x, -w_o.y, w_o.z};
                const Vec3 f{R / jtx::absCosTheta(w_i)};

                s = {f, w_i, p, isSpecular};
                return true;
            } else {
                // Transmission: sample BTDF
                Vec3 w_i;
                float etap;

                // Technically, this should always refract
                // Due to FP errors, we need to check anyway
                const bool valid = refract(w_o, {0, 0, 1}, eta, &etap, w_i);
                if (!valid) return false;

                const auto f = Vec3(T / jtx::absCosTheta(w_i));
                s = {f, w_i, 1 - p, isSpecular};
                return true;
            }
        }

        const Vec3 w_m = mf_.sampleWm(w_o, u);
        const float R = fresnelDielectric(w_o.dot(w_m), eta);
        const float T = 1 - R;

        // float pdf;
        const float p = R / (R + T);
        if (uc < p) {
            // Reflection
            const Vec3 w_i = reflect(w_o, w_m);
            if (!jtx::sameHemisphere(w_o, w_i)) return false;

            const float pdf = mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m)) * p;
            const auto f = mf_.D(w_m) * mf_.G(w_o, w_i) * R / (4 * jtx::absCosTheta(w_i) * jtx::absCosTheta(w_o));

            s = {Vec3(f), w_i, pdf, isSpecular};
            return true;
        } else {
            // Transmission
            float etap;
            Vec3 w_i;
            const bool totalInternalRefraction = !refract(w_o, w_m, eta, &etap, w_i);
            if (jtx::sameHemisphere(w_o, w_i) || w_i.z == 0 || totalInternalRefraction) return false;

            // PDF
            const float d = jtx::absdot(w_i, w_m) / jtx::sqr(w_i.dot(w_m) + w_o.dot(w_m) / etap);
            const float pdf = mf_.pdf(w_o, w_m) * d * (1 - p);

            // BRDF
            auto f = mf_.D(w_m) * T * mf_.G(w_o, w_i) * jtx::abs(w_i.dot(w_m) * w_o.dot(w_m));
            f /= jtx::sqr(w_i.dot(w_m) + w_m.dot(w_o) / etap) * jtx::abs(jtx::cosTheta(w_i) * jtx::cosTheta(w_o));

            s = {Vec3(f), w_i, pdf, isSpecular};
            return true;
        }
    }

    [[nodiscard]] float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        if (eta == 1 || mf_.smooth()) return 0;
        // Compute half-vector

        // $\cos\theta_o$ and $\cos\theta_i$ are the perpendicular components of the
        // incident and outgoing directions, respectively.
        const float cosTheta_o = jtx::absCosTheta(w_o);
        const float cosTheta_i = jtx::absCosTheta(w_i);
        // If $\cos\theta_i$ and $\cos\theta_o$ are on the same side of the surface
        // their product will be >0
        const bool reflect = cosTheta_o * cosTheta_i > 0;

        // We need to update eta depending on the type of ray interactions
        float etap = 1;
        // In the case of refraction, we need to update eta depending on the oritentation
        // of the rays
        if (!reflect) {
            // If $\cos\theta_o$ is > 0, the outgoing ray is on the same side as the normal
            // in which case the eta remains the same
            // If < 0, the outgoing ray is on the opposite side of the normal, so we flip eta
            etap = cosTheta_o > 0 ? eta : 1 / eta;
        }

        Vec3 w_m = w_i * etap + w_o;
        if (cosTheta_i == 0 || cosTheta_o == 0 || w_m.lenSqr() == 0) return 0;
        w_m = jtx::faceForward(w_m.normalize(), {0, 0, 1});

        // Discard back-facing microfacets
        // If the dot product is negative, they are back facing
        // However, we also need to respect the side of the surface. If $\omega_i$ or $\omega_o$ are on
        // the opposite side of the surface, we will need to negate the dot product.
        // We can do this by multiplying by their perpendicular component $\cos\theta$
        if (jtx::dot(w_m, w_i) * cosTheta_i < 0 || jtx::dot(w_m, w_o) * cosTheta_o < 0) return 0;

        // Determine fresnel reflectance and transmission
        const float R = fresnelDielectric(jtx::dot(w_o, w_m), eta);
        const float T = 1 - R;

        // Evaluate PDF
        float pdf;
        if (reflect) {
            pdf = mf_.pdf(w_o, w_m) / (4 * jtx::absdot(w_o, w_m)) * (R / (R + T));
        } else {
            const float d = jtx::absdot(w_i, w_m) / jtx::sqr(w_i.dot(w_m) + w_o.dot(w_m) / etap);
            pdf = mf_.pdf(w_o, w_m) * d * (T / (R + T));
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
