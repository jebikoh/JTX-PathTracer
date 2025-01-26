#pragma once

#include "../rt.hpp"
#include "../sampling.hpp"

bool isInf(float x) {
    return x == INF || x == -INF;
}

constexpr float TR_SMOOTH_THRESHOLD = 1e-3f;

class TrowbridgeReitz {
public:
    TrowbridgeReitz(float alphaX, float alphaY)
        : alphaX_(alphaX),
          alphaY_(alphaY) {}

    /**
     * Checks if the alpha values are low enough to be considered "effectively smooth"
     * If true, the surface can be treated as perfectly specular
     * @return true if effectively smooth, false otherwise
     */
    bool smooth() const {
        return jtx::max(alphaX_, alphaY_) < TR_SMOOTH_THRESHOLD;
    }

    /**
     * Evaluates the Trowbridge-Reitz microfacet distribution $D(\omega_m)$
     * @param w_m microfacet surface normal w_m
     * @return relative differential area of microfacets with surface normal w_m
     */
    float D(const Vec3 &w_m) const {
        float tan2Theta = jtx::tan2Theta(w_m);
        if (isInf(tan2Theta)) return 0;

        float e = tan2Theta * (jtx::sqr(jtx::cosPhi(w_m) / alphaX_) + jtx::sqr(jtx::sinPhi(w_m) / alphaY_));
        float cos4Theta = jtx::sqr(jtx::cos2Theta(w_m));

        return 1 / (PI * alphaX_ * alphaY_ * cos4Theta * jtx::sqr(1 + e));
    }

    /**
     * Evaluates the Trowbrige-Reitz distribution of visible normals $D_\omega(\omega_m)$
     * @param w viewing direction
     * @param wm microfacet normal
     * @return relative differential area of microfacets with surface normal w_m visible from w
     */
    float D_omega(const Vec3 &w, const Vec3 &wm) const {
        return G1(w) / jtx::absCosTheta(wm) * D(wm) * jtx::absdot(w, wm);
    }

    float pdf(const Vec3 &w, const Vec3 &wm) const {
        return D_omega(w, wm);
    }

    /**
     * Analytical solution of the auxiliary function for Trowbridge-Reitz
     * @param w direction
     * @return value of Lambda(w)
     */
    float lambda(const Vec3 &w) const {
        const float tan2Theta = jtx::tan2Theta(w);
        if (isInf(tan2Theta)) return 0;
        const float alpha2 = jtx::sqr(alphaX_ * jtx::cosPhi(w)) + jtx::sqr(alphaY_ * jtx::sinPhi(w));
        return 0.5 * (jtx::sqrt(1 + alpha2 * tan2Theta) - 1);
    }

    /**
     * Masking function
     * @param w direction
     * @return fraction of microfacets visible from direction w
     */
    float G1(const Vec3 &w) const { return 1 / (1 + lambda(w)); }

    /**
     * Masking-shadowing function
     * @param w_o out direction
     * @param w_i incident direction
     * @return fraction of microfacets visible from both w_o and w_i
     */
    float G(const Vec3 &w_o, const Vec3 &w_i) const {
        return 1 / (1 + lambda(w_o) + lambda(w_i));
    }

    Vec3 sampleW_m(Vec3 w, Vec2f u) const {
        // Transform to hemispherical configuration
        Vec3 wh = Vec3(alphaX_ * w.x, alphaY_ * w.y, w.z).normalize();
        if (wh.z < 0) wh = -wh;

        // Orthonormal basis
        const Vec3 t1 = (wh.z < 0.99999f) ? jtx::cross({0, 0, 1}, wh).normalize() : Vec3(1, 0,0);
        const Vec3 t2 = jtx::cross(wh, t1);

        // Sample uniform disk
        Vec2f p = sampleUniformDiskPolar(u);

        // Warp projection
        const float h = jtx::sqrt(1 - jtx::sqr(p.x));
        p.y = jtx::lerp(p.y, h, (1 + wh.z) / 2);

        // Re-project and transform normal
        const float pz = jtx::sqrt(jtx::max(0.0f, 1.0f - p.lenSqr()));
        const Vec3 n_h = p.x * t1 + p.y * t2 + pz * wh;
        return Vec3(alphaX_ * n_h.x, alphaY_ * n_h.y, jtx::max(1e-6f, n_h.z)).normalize();
    }
private:
    float alphaX_;
    float alphaY_;
};