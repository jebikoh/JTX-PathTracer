#include "bxdf.hpp"
#include "../material.hpp"
#include "diffuse.hpp"

BSDFSample sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u)  {
    if (mat->type == Material::DIFFUSE) {
        const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
        const auto bxdf         = DiffuseBRDF{mat->albedo};

        const auto w_o_local = sFrame.toLocal(w_o);
        if (w_o_local.z == 0) return {};
        auto bs = bxdf.sample(w_o_local, uc, u);
        bs.w_i  = sFrame.toWorld(bs.w_i);

        return bs;
    }

    return {};
}