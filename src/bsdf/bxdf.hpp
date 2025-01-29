#pragma once

#include "../material.hpp"
#include "../rt.hpp"
#include "../util/complex.hpp"

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
    const float r_parallel      = (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    const float r_perpendicular = (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);

    // Compute fresnel reflectance
    return (r_parallel * r_parallel + r_perpendicular * r_perpendicular) / 2;
}

/**
 * Computes the fresnel reflectance of conductive surfaces
 * @param cosTheta_i cos of incident angle
 * @param eta index of refraction: eta_t / eta_i
 * @return fresnel reflectance
 */
inline float fresnelComplex(float cosTheta_i, const Complex &eta) {
    // Clamp in case of floating point issues
    cosTheta_i = jtx::clamp(cosTheta_i, 0, 1);

    // Compute Snell's Law (with complex numbers)
    const float numerator = 1 - cosTheta_i * cosTheta_i;
    const auto radicand   = numerator / (eta * eta);
    const auto cosTheta_t = sqrt(1 - radicand);

    // Compute complex amplitudes
    const auto r_parallel      = (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    const auto r_perpendicular = (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);

    // Compute fresnel reflectance via norm
    return (norm(r_parallel) + norm(r_perpendicular)) / 2;
}

/**
 * Computes the fresnel reflectance of conductive surfaces for RGB colors
 * Requires IOR and k values for each color channel
 * @param cosTheta_i cos of incident angle
 * @param eta per-channel index of refraction: eta_t / eta_i
 * @param k per-channel extinction coefficient
 * @return fresnel reflectance
 */
inline Vec3 fresnelComplexRGB(const float cosTheta_i, const Vec3 &eta, const Vec3 &k) {
    Vec3 result;
    result.r = fresnelComplex(cosTheta_i, Complex(eta.r, k.r));
    result.g = fresnelComplex(cosTheta_i, Complex(eta.g, k.g));
    result.b = fresnelComplex(cosTheta_i, Complex(eta.b, k.b));
    return result;
}

struct BSDFSample {
    Vec3 fSample;
    Vec3 w_i;
    float pdf;
};

struct HitRecord;
struct Material;
bool sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u, BSDFSample &s);
