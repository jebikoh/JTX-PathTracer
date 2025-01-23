#pragma once

#include "rt.hpp"
#include "sampling.hpp"
#include <jtxlib/util/taggedptr.hpp>

class DiffuseBRDF;

struct BSDFSample {
    Vec3 sample;
    Vec3 w_i;
    float pdf;
};

class BxDF : public jtx::TaggedPtr<DiffuseBRDF> {
public:
    using TaggedPtr::TaggedPtr;

    Vec3 evaluate(Vec3 w_o, Vec3 w_i) const;

    BSDFSample sample(Vec3 w_o, float uc, Vec2f u) const;

    float pdf(Vec3 w_o, Vec3 w_i) const;

    Vec3 rhoHD(const Vec3 &w_o, const span<const float> uc, const span<const Vec2f> u2) const {
        Vec3 r;
        for (size_t i = 0; i < uc.size(); ++i) {
            auto bs = sample(w_o, uc[i], u2[i]);
            r += bs.sample * jtx::absCosTheta(bs.w_i) / bs.pdf;
        }
        return r / uc.size();
    }

    Vec3 rhoHH(const span<const Vec2f> u1, const span<const float> uc, const span<const Vec2f> u2) const {
        Vec3 r;
        for (size_t i = 0; i < uc.size(); ++i) {
            const Vec3 w_o = sampleUniformHemisphere(u1[i]);
            if (w_o.z == 0) continue;
            const float pdf_o = uniformHemispherePDF();
            auto bs = sample(w_o, uc[i], u2[i]);
            r += bs.sample * jtx::absCosTheta(bs.w_i) * jtx::absCosTheta(w_o) / (pdf_o * bs.pdf);
        }
        return r / (PI * uc.size());
    }
};

class DiffuseBRDF {
public:
    DiffuseBRDF(const Vec3 &R) : R_(R) {}

    Vec3 evaluate(Vec3 w_o, Vec3 w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return {};
        return R_ * INV_PI;
    }

    BSDFSample sample(Vec3 w_o, float uc, Vec2f u) const {
        Vec3 w_i = sampleCosineHemisphere(u);
        if (w_o.z < 0) w_i.z *= -1;
        const float pdf = cosineHemispherePDF(jtx::absCosTheta(w_i));
        return {R_ * INV_PI, w_i, pdf};
    }

    float pdf(Vec3 w_o, Vec3 w_i) const {
        if (!jtx::sameHemisphere(w_o, w_i)) return 0;
        return cosineHemispherePDF(jtx::absCosTheta(w_i));
    }
private:
    Vec3 R_;
};

class BSDF {
public:
    BSDF() = default;
    BSDF(const Vec3 &n_s, const Vec3 &dp_du, const BxDF &bxdf) : bxdf(bxdf), frame(jtx::Frame::fromXZ(jtx::normalize(dp_du), n_s)) {};

    Vec3 renderToLocal(const Vec3 &v) const { return frame.toLocal(v); }
    Vec3 localToRender(const Vec3 &v) const { return frame.toWorld(v); }

    Vec3 evaluate(const Vec3 &w_o, const Vec3 &w_i) const {
        const Vec3 o = renderToLocal(w_o);
        const Vec3 i = renderToLocal(w_i);
        if (o.z == 0) return {};
        return bxdf.evaluate(o, i);
    }

    BSDFSample sample(const Vec3 &w_o, const float u, const Vec2f &u2) const {
        const Vec3 o = renderToLocal(w_o);
        if (o.z == 0) return {};
        auto bs = bxdf.sample(o, u, u2);
        bs.w_i = localToRender(bs.w_i);
        return bs;
    }

    float pdf(const Vec3 &w_o, const Vec3 &w_i) const {
        const Vec3 o = renderToLocal(w_o);
        const Vec3 i = renderToLocal(w_i);
        if (o.z == 0) return {};
        return bxdf.pdf(o, i);
    }

private:
    BxDF bxdf;
    jtx::Frame frame;
};

Vec3 BxDF::evaluate(Vec3 w_o, Vec3 w_i) const {
    auto fn = [&](auto ptr) { return ptr->evaluate(w_o, w_i); };
    return dispatch(fn);
}

BSDFSample BxDF::sample(Vec3 w_o, float uc, Vec2f u) const {
    auto fn = [&](auto ptr) { return ptr->sample(w_o, uc, u); };
    return dispatch(fn);
}

float BxDF::pdf(Vec3 w_o, Vec3 w_i) const {
    auto fn = [&](auto ptr) { return ptr->pdf(w_o, w_i); };
    return dispatch(fn);
}