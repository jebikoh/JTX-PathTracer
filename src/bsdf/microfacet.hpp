#pragma once

#include "../rt.hpp"

bool isInf(float x) {
    return x == INF || x == -INF;
}

constexpr float TR_SMOOTH_THRESHOLD = 1e-3f;

class TrowbridgeReitz {
public:
    TrowbridgeReitz(float alphaX, float alphaY)
        : alphaX_(alphaX),
          alphaY_(alphaY) {}

    bool smooth() const {
        return jtx::max(alphaX_, alphaY_) < TR_SMOOTH_THRESHOLD;
    }

    /**
     * Evaluates the Trowbridge-Reitz microfacet distribution D(w_m)
     * @param w_m microfacet surface normal w_m
     * @return relative differential area of microfacets with surface normal w_m
     */
    float evaluate(const Vec3 &w_m) const {
        float tan2Theta = jtx::tan2Theta(w_m);
        if (isInf(tan2Theta)) return 0;

        float e = tan2Theta * (jtx::sqr(jtx::cosPhi(w_m) / alphaX_) + jtx::sqr(jtx::sinPhi(w_m) / alphaY_));
        float cos4Theta = jtx::sqr(jtx::cos2Theta(w_m));

        return 1 / (PI * alphaX_ * alphaY_ * cos4Theta * jtx::sqr(1 + e));
    }

    /**
     * Analytical solution of the auxiliary function for Trowbridge-Reitz
     * @param w direction
     * @return value of Lambda(w)
     */
    float lambda(const Vec3 &w) const {
        float tan2Theta = jtx::tan2Theta(w);
        if (isInf(tan2Theta)) return 0;
        float alpha2 = jtx::sqr(alphaX_ * jtx::cosPhi(w)) + jtx::sqr(alphaY_ * jtx::sinPhi(w));
        return 0.5 * (jtx::sqrt(1 + alpha2 * tan2Theta) - 1);
    }

    /**
     * Masking function
     * @param w direction
     * @return fraction of microfacets visible from direction w
     */
    float g1(const Vec3 &w) const { return 1 / (1 + lambda(w)); }

    /**
     * Masking-shadowing function
     * @param w_o out direction
     * @param w_i incident direction
     * @return fraction of microfacets visible from both w_o and w_i
     */
    float g(const Vec3 &w_o, const Vec3 &w_i) const {
        return 1 / (1 + lambda(w_o) + lambda(w_i));
    }
private:
    float alphaX_;
    float alphaY_;
};