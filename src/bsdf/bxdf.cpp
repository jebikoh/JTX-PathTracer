#include "bxdf.hpp"
#include "../material.hpp"
#include "diffuse.hpp"
#include "conductor.hpp"

BSDFSample sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u)  {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local = sFrame.toLocal(w_o);
    if (w_o_local.z == 0) return {};

    if (mat->type == Material::DIFFUSE) {
        const auto bxdf         = DiffuseBRDF{mat->albedo};
        auto bs = bxdf.sample(w_o_local, uc, u);
        bs.w_i  = sFrame.toWorld(bs.w_i);

        return bs;
    }

    if (mat->type == Material::CONDUCTOR) {
        const auto bxdf = ConductorBRDF{mat->IOR, mat->k};
        auto bs = bxdf.sample(w_o_local, uc, u);
        bs.w_i  = sFrame.toWorld(bs.w_i);

        return bs;
    }

    return {};
}