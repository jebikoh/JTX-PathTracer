#pragma once
#include <jtx.hpp>

namespace jtx {

constexpr float JTX_GGX_SPECULAR_THRESHOLD = 1e-3f;

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
     * Checks if the roughness parameters are low enough to be considered specular.
     * @return True if surface can be considered specular, false otherwise.
     */
    bool IsSpecular() const {
        return m_alpha.max() < JTX_GGX_SPECULAR_THRESHOLD;
    }

    /**
     * Evaluates the microfacet distribution function D for a given microfacet normal.
     *
     * Integrating the projected area of the microfacet distribution function about the
     * surface normal over the hemisphere H_2(n) should yield 1.
     * @param wm microfacet normal in local shading space.
     * @return relative differential area (density) with microfacet wm.
     */
    float D(const vec3 &wm) const {
        const float cos2Theta = Cos2Theta(wm);
        const float cos4Theta = Sqr(cos2Theta);
        const float tan2Theta = Tan2Theta(wm, cos2Theta);

        const float a = JTX_PI_F * m_alpha.x * m_alpha.y * cos4Theta;
        const float b = 1.0f + tan2Theta * (Sqr(CosPhi(wm) / m_alpha.x) + Sqr(SinPhi(wm) / m_alpha.y));

        return 1 / (a * Sqr(b));
    }

    /**
     * Analytic solution for the auxiliary function for the Smith approximation of the
     * masking term G1.
     * @param w direction in local shading space
     * @return the auxiliary function Lambda(w)
     */
    float Lambda(const vec3 &w) const {
        const float alpha2 = Sqr(m_alpha.x * CosPhi(w)) + Sqr(m_alpha.y * SinPhi(w));
        const float tan2Theta = Tan2Theta(w);
        return 0.5 * (Sqrt(1 + alpha2 * tan2Theta) - 1.0f);
    }

    /**
     * Smith approximation of the masking term G1.
     *
     * Calculates the fraction of total projected microfacet area visible from direction w.
     * @param w direction in local shading space
     * @return the fraction of microfacets visible from w
     */
    float G1(const vec3 &w) const {
        return 1 / (1 + Lambda(w));
    }

    /**
     * Shadowing-masking term G.
     *
     * Calculates the fraction of total projected microfacet area that is visible from both directions.
     * NOTE: this is the height-correlated version of the Smith approximation.
     * @param wo view direction in local shading space
     * @param wi incident direction in local shading space
     * @return the fraction of microfacets visible from both directions
     */
    float G(const vec3 &wo, const vec3 &wi) const {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }

    /**
     * Evaluates the microfacet distribution of visible normals, D_w(wm).
     *
     * This is the distribution of microfacet normals wm that are visible from direction w.
     * @param w direction in local shading space
     * @param wm microfacet normal in local shading space
     * @return relative differential area (density) of microfacets visible from w with normal wm
     */
    float D(const vec3 &w, const vec3 &wm) const {
        return G1(w) * D(wm) * AbsDot(w, wm) / AbsCosTheta(w);
    }

    /**
     * PDF of the distribution of visible normals, D_w(wm).
     *
     * Since D_w(wm) is normalized, this is just equivalent to D(w, wm)
     * @param w
     * @param wm
     * @return
     */
    float PDF(const vec3 &w, const vec3 &wm) const {
        return D(w, wm);
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
        const vec3 vh = Normalize(vec3(m_alpha, 1.0f) * w);

        // ONB
        const float lengthSqr = vh.x * vh.x + vh.y * vh.y;
        const vec3 t1 = lengthSqr > 0 ? vec3(-vh.y, vh.x, 0) * (1.0f / Sqrt(lengthSqr)) : vec3(1, 0, 0);
        const vec3 t2 = Cross(vh, t1);


    }
private:
    vec2 m_alpha; // Roughness parameters (alpha_x, alpha_y)
};

}