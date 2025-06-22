#pragma once
#include "util/sampling.hpp"


#include <jtx.hpp>

namespace jtx {

constexpr float JTX_GGX_SPECULAR_THRESHOLD = 1e-3f;

inline bool IsInf(const float x) {
    return std::isinf(x);
}

/**
 * Trowbridge-Reitz (GGX) microfacet model for rough surfaces.
 *
 * Parameterized by X and Y roughness values:
 *  - If they are equal, the surface is isotropic.
 *  - If they differ, the surface is anisotropic.
 */
class GGX {
public:
    explicit GGX(const vec2 &alpha) : m_alpha(alpha) {}

    /**
     * Checks if the roughness parameters are low enough to be considered smooth.
     *
     * In these cases, it is more efficient and numerically stable to evaluate the surface as perfectly specular
     * @return True if surface can be considered smooth, false otherwise.
     */
    bool IsSmooth() const {
        return m_alpha.Max() < JTX_GGX_SPECULAR_THRESHOLD;
    }

    /**
     * Evaluates the microfacet normal distribution function D for a given microfacet normal.
     *
     * Integrating the projected area of the microfacet distribution function about the
     * surface normal over the hemisphere H_2(n) should yield 1.
     * @param wm microfacet normal in local shading space.
     * @return relative differential area (density) with microfacet wm.
     */
    float EvaluateNDF(const vec3 &wm) const {
        const float cos2Theta = Cos2Theta(wm);
        const float tan2Theta = Tan2Theta(wm, cos2Theta);

        // We have to disable fp:fast and --fast-math to use this
        // TODO: explore optimization from toggling fast math for this function
        if (IsInf(tan2Theta)) return 0.0f;

        const float cos4Theta = Sqr(cos2Theta);

        const float a = JTX_PI_F * m_alpha.x * m_alpha.y * cos4Theta;
        const float b = 1.0f + tan2Theta * (Sqr(CosPhi(wm) / m_alpha.x) + Sqr(SinPhi(wm) / m_alpha.y));

        return 1 / (a * Sqr(b));
    }

    /**
     * Evaluates analytic solution for the GGX auxiliary function for the Smith approximation.
     * @param w direction in local shading space
     * @return the auxiliary function Lambda(w)
     */
    float Lambda(const vec3 &w) const {
        const float tan2Theta = Tan2Theta(w);
        if (IsInf(tan2Theta)) return 0.0f;

        const float alpha2 = Sqr(m_alpha.x * CosPhi(w)) + Sqr(m_alpha.y * SinPhi(w));
        return 0.5 * (Sqrt(1 + alpha2 * tan2Theta) - 1.0f);
    }

    /**
     * Evaluates smith approximation of the masking function G1.
     *
     * Calculates the fraction of total projected microfacet area visible from direction w.
     * @param w direction in local shading space
     * @return the fraction of microfacets visible from w
     */
    float EvaluateMasking(const vec3 &w) const {
        return 1 / (1 + Lambda(w));
    }

    /**
     * Evaluates smith approximation of shadowing-masking function G.
     *
     * Calculates the fraction of total projected microfacet area that is visible from both directions.
     * @param wo view direction in local shading space
     * @param wi incident direction in local shading space
     * @return the fraction of microfacets visible from both directions
     */
    float EvaluateShadowingMasking(const vec3 &wo, const vec3 &wi) const {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }

    /**
     * Evaluates the visible microfacet normal distribution function, D_w(wm).
     *
     * This is the distribution of microfacet normals wm that are visible from direction w.
     * @param w direction in local shading space
     * @param wm microfacet normal in local shading space
     * @return relative differential area (density) of microfacets visible from w with normal wm
     */
    float EvaluateVNDF(const vec3 &w, const vec3 &wm) const {
        // G1(w) / cos(theta) * D(wm) * max(0, dot(w, wm))
        return EvaluateMasking(w) / AbsCosTheta(w) * EvaluateNDF(wm) * AbsDot(w, wm);
    }

    /**
     * Calculates PDF of the visible normal distribution, D_w(wm).
     *
     * Since D_w(wm) is normalized, this is just equivalent to D(w, wm)
     * @param w
     * @param wm
     * @return
     */
    float PDF(const vec3 &w, const vec3 &wm) const {
        return EvaluateVNDF(w, wm);
    }

    /**
     * Samples a microfacet normal wm from the distribution of visible normals D_w(wm).
     *
     * Uses the Heitz 2018 method
     * @param w direction in local shading space
     * @param s random sample in [0, 1]^2
     * @return sampled microfacet normal wm in local shading space
     */
    vec3 SampleWm(const vec3 &w, const vec2 &s) const {
        // Transform view direction to hemispherical configuration
        // This is done applying the scaling factors and re-normalizing
        vec3 vh = Normalize(vec3(m_alpha, 1.0f) * w);
        if (vh.z < 0.0f) vh = -vh; // Ensure we are in the upper hemisphere

        // ONB
        const float lengthSqr = vh.x * vh.x + vh.y * vh.y;
        // If lenSqr is 0, then the z component of vh is 1 -- we can use the default tangent
        const vec3 t1 = lengthSqr > 0 ? vec3(-vh.y, vh.x, 0) * (1.0f / Sqrt(lengthSqr)) : vec3(1, 0, 0);
        const vec3 t2         = Cross(vh, t1);

        // Sample a point on a unit disc
        vec2 p = SampleUniformDiscPolar(s); // Why polar and not concentric?

        // Apply affine transformation to the sampled point
        const float h = Sqrt(1 - p.x * p.x);
        const float scale = 0.5 * (1 + vh.z);
        p.y = (1.0f - scale) * h + scale * p.y; // Scale and offset via linear interpolation

        // Project the point onto the hemisphere
        const vec3 nh = p.x * t1 + p.y * t2 + Sqrt(Max(0.0f, 1.0f - p.x * p.x - p.y * p.y)) * vh;

        // Reapply the scaling factors
        return Normalize(vec3(m_alpha.x * nh.x, m_alpha.y * nh.y, Max(1e-6f, nh.z)));
    }

private:
    vec2 m_alpha; // Roughness parameters (alpha_x, alpha_y)
};

}