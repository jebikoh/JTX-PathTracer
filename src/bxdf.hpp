#pragma once

#include "material.hpp"
#include "rt.hpp"
#include "sampling.hpp"

inline Vec3 reflect(const Vec3 &w_o, const Vec3 &n) {
    return -w_o + 2 * jtx::dot(w_o, n) * n;
}

/**
 * Computes refraction direction w_i via Snell's Law.
 * @param w_i incident direction
 * @param n surface normal
 * @param eta relative index of refraction: eta_t / eta_i
 * @param eta_p relative eta used after checking ray direction
 * @param w_t refraction direction
 * @return true in case refraction, false in case total internal reflection
 */
inline bool refract(const Vec3 &w_i, Vec3 n, float eta, float *eta_p, Vec3 &w_t) {
    float cosTheta_i = jtx::dot(w_i, n);

    // Check if ray is exiting the surface
    // If so, flip parameters
    if (cosTheta_i < 0) {
        eta        = 1 / eta;
        cosTheta_i = -cosTheta_i;
        n          = -n;
    }

    // Update eta for user if requested
    if (eta_p) *eta_p = eta;

    // Compute Snell's Law
    const float radicand = jtx::max(0.0f, 1 - jtx::sqr(cosTheta_i)) / (eta * eta);
    if (radicand >= 1) return false;
    const float cosTheta_t = jtx::safeSqrt(1 - radicand);

    w_t = -w_i / eta + (cosTheta_i / eta - cosTheta_t) * n;
    return true;
}

/**
 * Computes the fresnel reflectance of dielectric surfaces
 * @param cosTheta_i cos of incident angle
 * @param eta relative index of refraction: eta_t / eta_i
 * @return fresnel reflectance
 */
inline float fresnelDielectric(float cosTheta_i, float eta) {
    // Clamp in case of floating point issues
    cosTheta_i = jtx::clamp(cosTheta_i, -1, 1);

    // Similar to Snell's Law, we need to flip eta and cosTheta_i if our ray is exiting
    if (cosTheta_i < 0) {
        eta        = 1 / eta;
        cosTheta_i = -cosTheta_i;
    }

    // Compute Snell's Law
    const float radicand = (1 - cosTheta_i * cosTheta_i) / (eta * eta);
    if (radicand >= 1) return 1.0f;
    const float cosTheta_t = jtx::safeSqrt(1 - radicand);

    // Compute amplitudes
    float r_parallel      = (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    float r_perpendicular = (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);

    // Compute fresnel reflectance
    return (r_parallel * r_parallel + r_perpendicular * r_perpendicular) / 2;
}

struct BSDFSample {
    Vec3 fSample;
    Vec3 w_i;
    float pdf;
};

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

inline BSDFSample sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u) {
    if (mat->type == Material::DIFFUSE) {
        const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
        const auto bxdf         = DiffuseBRDF{mat->albedo};

        const auto w_o_local = sFrame.toLocal(w_o);
        if (w_o_local.z == 0) return {};
        auto bs = bxdf.sample(w_o_local, uc, u);
        bs.w_i = sFrame.toWorld(bs.w_i);

        return bs;
    }

    return {};
}