#pragma once

#include "material.hpp"
#include "rt.hpp"
#include "sampling.hpp"
#include <jtxlib/util/taggedptr.hpp>

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

    [[nodiscard]]
    Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return {};
        return R_ * INV_PI;
    }

    [[nodiscard]]
    BSDFSample sample(const Vec3 &w_o, float uc, const Vec2f &u) const {
        Vec3 w_i = sampleCosineHemisphere(u);
        if (w_o.z < 0) w_i.z *= -1;
        const float pdf = cosineHemispherePDF(jtx::absCosTheta(w_i));
        return {R_ * INV_PI, w_i, pdf};
    }

    [[nodiscard]]
    float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return 0;
        return cosineHemispherePDF(jtx::absCosTheta(w_i));
    }

private:
    Vec3 R_;
};

BSDFSample sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u) {
    if (mat->type == Material::DIFFUSE) {
//        std::cout << "Evaluating Diffuse BRDF" << std::endl;
//        std::cout << "w_o: " << toString(w_o) << std::endl;
//        std::cout << "uc: " << uc << std::endl;
//        std::cout << "u: " << toString(u) << std::endl;
//        std::cout << "-----" << std::endl;
//        std::cout << "n: " << toString(rec.normal) << std::endl;
//        std::cout << "t: " << toString(rec.tangent) << std::endl;
//        std::cout << "bt: " << toString(rec.bitangent) << std::endl;
//        std::cout << "-----------------------" << std::endl;

        jtx::Frame sFrame = jtx::Frame::fromY(rec.normal);
        auto bxdf = DiffuseBRDF{mat->albedo};

        auto w_o_local = sFrame.toLocal(w_o);
        if (w_o_local.z == 0) return {};
        auto bs = bxdf.sample(w_o_local, uc, u);
        bs.w_i = sFrame.toWorld(bs.w_i);

        return bs;
    }

    return {};
}

//
//class BxDF : public jtx::TaggedPtr<DiffuseBRDF> {
//public:
//    using TaggedPtr::TaggedPtr;
//
//    [[nodiscard]]
//    Vec3 evaluate(Vec3 w_o, Vec3 w_i) const;
//
//    [[nodiscard]]
//    BSDFSample sample(Vec3 w_o, float uc, Vec2f u) const;
//
//    [[nodiscard]]
//    float pdf(Vec3 w_o, Vec3 w_i) const;
//
//    [[nodiscard]]
//    Vec3 rhoHD(const Vec3 &w_o, const span<const float> uc, const span<const Vec2f> u2) const {
//        Vec3 r;
//        for (size_t i = 0; i < uc.size(); ++i) {
//            auto bs = sample(w_o, uc[i], u2[i]);
//            r += bs.sample * jtx::absCosTheta(bs.w_i) / bs.pdf;
//        }
//        return r / uc.size();
//    }
//
//    [[nodiscard]]
//    Vec3 rhoHH(const span<const Vec2f> u1, const span<const float> uc, const span<const Vec2f> u2) const {
//        Vec3 r;
//        for (size_t i = 0; i < uc.size(); ++i) {
//            const Vec3 w_o = sampleUniformHemisphere(u1[i]);
//            if (w_o.z == 0) continue;
//            const float pdf_o = uniformHemispherePDF();
//            auto bs           = sample(w_o, uc[i], u2[i]);
//            r += bs.sample * jtx::absCosTheta(bs.w_i) * jtx::absCosTheta(w_o) / (pdf_o * bs.pdf);
//        }
//        return r / (PI * uc.size());
//    }
//};

//
//class BSDF {
//public:
//    BSDF() = default;
//    BSDF(const Vec3 &n_s, const Vec3 &dp_du, const BxDF &bxdf)
//        : bxdf(bxdf),
//          frame(jtx::Frame::fromXZ(jtx::normalize(dp_du), n_s)) {};
//    BSDF(const jtx::Frame &frame, const BxDF &bxdf)
//        : bxdf(bxdf),
//          frame(frame) {}
//
//    Vec3 renderToLocal(const Vec3 &v) const { return frame.toLocal(v); }
//    Vec3 localToRender(const Vec3 &v) const { return frame.toWorld(v); }
//
//    Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
//        const Vec3 o = renderToLocal(w_o);
//        const Vec3 i = renderToLocal(w_i);
//        if (o.z == 0) return {};
//        return bxdf.evaluate(o, i);
//    }
//
//    BSDFSample sample(const Vec3 &w_o, const float u, const Vec2f &u2) const {
//        const Vec3 o = renderToLocal(w_o);
//        if (o.z == 0) return {};
//        auto bs = bxdf.sample(o, u, u2);
//        bs.w_i  = localToRender(bs.w_i);
//        return bs;
//    }
//
//    float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
//        const Vec3 o = renderToLocal(w_o);
//        const Vec3 i = renderToLocal(w_i);
//        if (o.z == 0) return {};
//        return bxdf.pdf(o, i);
//    }
//
//private:
//    BxDF bxdf;
//    jtx::Frame frame;
//};
//
//inline Vec3 BxDF::evaluate(Vec3 w_o, Vec3 w_i) const {
//    auto fn = [&](auto ptr) { return ptr->evaluate(w_o, w_i); };
//    return dispatch(fn);
//}
//
//inline BSDFSample BxDF::sample(Vec3 w_o, float uc, Vec2f u) const {
//    auto fn = [&](auto ptr) { return ptr->sample(w_o, uc, u); };
//    return dispatch(fn);
//}
//
//inline float BxDF::pdf(Vec3 w_o, Vec3 w_i) const {
//    auto fn = [&](auto ptr) { return ptr->pdf(w_o, w_i); };
//    return dispatch(fn);
//}