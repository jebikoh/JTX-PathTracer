#include "bxdf.hpp"
#include "../material.hpp"
#include "conductor.hpp"
#include "dielectric.hpp"
#include "diffuse.hpp"

bool sampleBxdf(const Material *mat, const HitRecord &rec, const Vec3 &w_o, float uc, const Vec2f &u, BSDFSample &s)  {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local = sFrame.toLocal(w_o);
    if (w_o_local.z == 0) return false;

    if (mat->type == Material::DIFFUSE) {
        const auto bxdf         = DiffuseBxDF{mat->albedo};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (mat->type == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{mat->alphaX, mat->alphaY}, mat->IOR, mat->k};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (mat->type == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF(mat->IOR.x);
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    return false;
}