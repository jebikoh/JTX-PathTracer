#pragma once
#include "util/complex.hpp"


#include <jtx.hpp>

namespace jtx {
struct SurfaceAttributes;
struct Scene;

struct BxDFSample {
    vec3 f;    // Scattering function value
    vec3 wi;   // Sampled incident direction
    float pdf; // PDF of the sampled direction
    float eta; // Relative index of refraction (eta_t / eta_i)
    bool bSpecular;
    bool bTransmission;
};

bool SampleBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s);
vec3 EvalBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, const vec3 &wi);
float PDFBxDF(const Scene &scene, const SurfaceAttributes &surface, const vec3 &wo, const vec3 &wi);

/**
 * Computes reflection direction of wo across surface with normal n.
 * @param wo outgoing direction
 * @param n surface normal
 * @return reflection direction
 */
inline vec3 Reflect(const vec3 &wo, const vec3 &n) {
    return -wo + 2.0f * jtx::Dot(wo, n) * n;
}

/**
 * Computes refraction direction wi via Snell's law.
 * @param wi incident direction
 * @param n surface normal
 * @param eta relative index of refraction (eta_t / eta_i). Will be updated if the ray is exiting the medium.
 * @param wt refraction direction (output)
 * @return true if refraction occurs, false if total internal reflection occurs
 */
inline bool Refract(const vec3 &wi, vec3 n, float &eta, vec3 &wt) {
    float cosThetaI = jtx::Dot(wi, n);

    // Ray is exiting, flip parameters
    //
    if (cosThetaI < 0.0f) {
        eta = 1 / eta;
        cosThetaI = -cosThetaI;
        n = -n;
    }

    // Compute Snell's law
    const float r = jtx::Max(0.0f, 1 - (cosThetaI * cosThetaI)) / (eta * eta);
    if (r >= 1) return false;
    const float cosThetaT = jtx::SafeSqrt(1 - r);

    wt = -wi / eta + (cosThetaI / eta - cosThetaT) * n;
    return true;
}

/**
 * Computes Schlick's approximation of fresnel reflectance.
 * @param wo outgoing direction
 * @param n surface normal
 * @param R reflectance color (RGB)
 * @return Fresnel reflectance value
 */
inline vec3 Schlick(const vec3 &wo, const vec3 &n, const vec3 &R) {
    const auto cosTheta = jtx::AbsDot(wo, n);
    const auto m = 1 - cosTheta;
    const auto m2 = m * m;
    return R + (vec3(1.0f) - R) * m2 * m2 * m;
}

/**
 * Computes the Fresnel reflectance for a dielectric material
 * @param cosThetaI incident cosine angle
 * @param eta relative index of refraction (eta_t / eta_i)
 * @return Fresnel reflectance
 */
inline float Fresnel(float cosThetaI, float eta) {
    cosThetaI = Clamp(cosThetaI, -1.0f, 1.0f);
    // Ray is exiting, flip parameters
    if (cosThetaI < 0.0f) {
        eta = 1 / eta;
        cosThetaI = -cosThetaI;
    }

    // Snell's law
    const float r = jtx::Max(0.0f, 1 - cosThetaI * cosThetaI) / (eta * eta);
    if (r >= 1) return 1.0f; // Total internal reflection
    const float cosThetaT = jtx::SafeSqrt(1 - r);

    // Amplitudes
    const float rParallel = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    const float rPerpendicular = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);

    return (rParallel * rParallel + rPerpendicular * rPerpendicular) / 2.0f;
}

/**
 * Computes the Fresnel reflectance for complex extinction coefficients.
 * @param cosThetaI incident cosine angle
 * @param eta relative index of refraction (eta_t / eta_i) as a complex number
 * @return Fresnel reflectance
 */
inline float FresnelComplex(float cosThetaI, const Complex &eta) {
    cosThetaI = Clamp(cosThetaI, 0.0f, 1.0f);

    const float n = 1 - cosThetaI * cosThetaI;
    const auto r = n / (eta * eta);
    const auto cosThetaT = Sqrt(1 - r);

    const auto rParallel = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    const auto rPerpendicular = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);

    return (Norm(rParallel) + Norm(rPerpendicular)) / 2.0f;
};

/**
 * Computes the Fresnel reflectance for complex extinction coefficients on RGB wavelengths
 * @param cosThetaI incident cosine angle
 * @param eta relative index of refraction (eta_t / eta_i) for RGB wavelengths
 * @param k complex extinction coefficients for RGB wavelengths
 * @return per-channel Fresnel reflectance
 */
inline vec3 FresnelComplexRGB(const float cosThetaI, const vec3 &eta, const vec3 &k) {
    vec3 result;
    result.r = FresnelComplex(cosThetaI, Complex(eta.r, k.r));
    result.g = FresnelComplex(cosThetaI, Complex(eta.g, k.g));
    result.b = FresnelComplex(cosThetaI, Complex(eta.b, k.b));
    return result;
}

}