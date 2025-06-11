#pragma once
#include <jtx.hpp>

namespace jtx {

struct BxDFSample {
    vec3 f;
    vec3 wi;
    vec3 pdf;
};

bool SampleBxDF();
vec3 EvalBxDF();
float PDFBxDF();

inline vec3 Reflect(const vec3 &wo, const vec3 &n) {
    return -wo + 2.0f * jtx::dot(wo, n) * n;
}

/**
 * Computes refraction direction wi via Snell's law.
 * @param wi incident direction
 * @param n surface normal
 * @param eta relative index of refraction (eta_t / eta_i)
 * @param wt refraction direction (output)
 * @return true if refraction occurs, false if total internal reflection occurs
 */
inline bool Refract(const vec3 &wi, vec3 n, float eta, vec3 &wt) {
    float cosThetaI = jtx::dot(wi, n);

    // Ray is exiting, flip parameters
    if (cosThetaI > 0.0f) {
        eta = 1 / eta;
        cosThetaI = -cosThetaI;
        n = -n;
    }

    // Compute Snell's law
    const float r = jtx::max(0.0f, 1 - (cosThetaI * cosThetaI) / (eta * eta));
    if (r >= 1) return false;
    const float cosThetaT = jtx::safeSqrt(1 - r);

    wt = -wi / eta + (cosThetaI / eta - cosThetaT) * n;
    return true;
}

inline vec3 Shlick(const vec3 &wo, const vec3 &wm, const vec3 &R) {
    const auto cosTheta = jtx::absdot(wo, wm);
    const auto m = 1 - cosTheta;
    const auto m2 = m * m;
    return R + (vec3(1.0f) - R) * m2 * m2 * m;
}

}