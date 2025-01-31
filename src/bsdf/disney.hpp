#pragma once

#include "microfacet.hpp"


/**
 * Disney uses slightly different notation from PBR 4th ed.
 *  - l -> wi
 *  - v -> wo
 *  - h -> w_m
 *
 *  - cosThetaD -> n * wi = n * l
 *  - cosThetaH -> n * wm = n * h
 *  - cosThetaO -> n * wo = n * v
 */

// Generalized Trowbridge-Reitz (GTR)
// Distribution Functions
// https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
inline float GTR1(const float cosThetaH, const float alpha) {
    if (alpha >= 1) return INV_PI;
    const float a2 = alpha * alpha;
    const float t  = 1 + (a2 - 1) * cosThetaH * cosThetaH;
    return (a2 - 1) / (PI * jtx::log(a2) * t);
}

inline float GTR2(const float cosThetaH, const float alpha) {
    const float a2 = alpha * alpha;
    const float t  = 1 + (a2 - 1) * cosThetaH * cosThetaH;
    return a2 / (PI * t * t);
}

inline float GTR2a(const float hDotX, const float hDotY, const float cosThetaH, const float alphaX, const float alphaY) {
    return 1 / (PI * alphaX * alphaY * jtx::sqr(jtx::sqr(hDotX / alphaX) + jtx::sqr(hDotY / alphaY) + cosThetaH * cosThetaH));
}

inline float schlickFresnel(const float u) {
    const float m  = jtx::clamp(1 - u, 0, 1);
    const float m2 = m * m;
    return m2 * m2 * m;
}

// Sampling functions for GTR
inline Vec3 sampleGTR1(const Vec2f &uc, float alpha) {
    alpha          = jtx::max(alpha, 1e-3f);
    const float a2 = alpha * alpha;

    const float phi    = 2 * PI * uc[0];
    const float sinPhi = jtx::sin(phi);
    const float cosPhi = jtx::cos(phi);

    const float cosTheta = jtx::sqrt(1.0f - jtx::pow(a2, 1 - uc[1]) / (1 - a2));
    const float sinTheta = jtx::sqrt(jtx::max(0.0f, 1.0f - cosTheta * cosTheta));

    return {sinTheta * cosPhi, sinTheta * sinPhi, cosTheta};
}

inline Vec3 sampleGTR2(const Vec2f &uc, float alpha) {
    alpha = jtx::max(alpha, 1e-3f);

    const float phi    = 2 * PI * uc[0];
    const float sinPhi = jtx::sin(phi);
    const float cosPhi = jtx::cos(phi);

    const float cosTheta = jtx::sqrt((1 - uc[1]) / (1 + (alpha * alpha - 1) * uc[1]));
    const float sinTheta = jtx::sqrt(jtx::max(0.0f, 1.0f - cosTheta * cosTheta));

    return {sinTheta * cosPhi, sinTheta * sinPhi, cosTheta};
}

inline Vec3 sampleGTR2a(const Vec2f &uc, const float alphaX, const float alphaY) {
    const float phi = 2 * PI * uc[0];

    const float sinPhi   = alphaY * jtx::sin(phi);
    const float cosPhi   = alphaX * jtx::cos(phi);
    const float tanTheta = jtx::sqrt(uc[1] / (1 - uc[1]));

    return {cosPhi * tanTheta, sinPhi * tanTheta, 1.0f};
}

struct DisneyMaterial {
    float roughness;
    Vec3 baseColor;
    float subsurface;
};

inline Vec3 DisneyDiffuse(const Vec3 &wo, const Vec3 &wi, const Vec3 &wm, const DisneyMaterial &mat) {
    // Using notation similar to that found here:
    // https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
    const float NDotL = jtx::absCosTheta(wi);
    const float NDotV = jtx::absCosTheta(wo);
    const float LDotH = jtx::dot(wi, wm);

    const float fss90 = LDotH * LDotH * mat.roughness;

    const float Rr    = 2 * fss90;
    const float FL     = schlickFresnel(NDotL);
    const float FV     = schlickFresnel(NDotV);
    const float fRetro = Rr * (FL + FV + FL * FV * (Rr - 1));
    const float fd     = (1 - 0.5 * FL) * (1 - 0.5 * FV) + fRetro;

    // Subsurface
    // This is basically a LERP but we can simplify
    const float Fss = (1 + (fss90 - 1) * FL) * (1 + (fss90 - 1) * FV);
    const float fss = 1.25f * (Fss * (1 / (NDotL * NDotV) - 0.5f) + 0.5f);

    // Mix subsurface and diffuse based on subsurface/flatness attribute
    return INV_PI * mat.baseColor * jtx::lerp(fd, fss, mat.subsurface);
}