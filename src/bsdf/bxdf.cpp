#include "bxdf.hpp"
#include "../material.hpp"
#include "../scene.hpp"
#include "diffuse.hpp"
#include "conductor.hpp"
#include "dielectric.hpp"

bool sampleBxdf(const Scene &scene, const SurfaceIntersection &rec, const Vec3 &w_o, const float uc, const Vec2f &u, BSDFSample &s) {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local    = sFrame.toLocal(w_o);
    if (w_o_local.z == 0) return false;

    if (rec.material->mType == Material::DIFFUSE) {
        // If material has a diffuse texture, we should use that
        Vec3 albedo = rec.material->parameters.albedo;
        if (rec.material->textureIndices.albedo != -1) {
            albedo = scene.textures[rec.material->textureIndices.albedo].getTexel(rec.uv);
            albedo = sRGBToLinear(albedo);
        }

        const auto bxdf = DiffuseBxDF{albedo};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (rec.material->mType == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior, rec.material->parameters.k};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (rec.material->mType == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior.x);
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    return false;
}

Vec3 evalBxdf(const Scene &scene, const Material *mat, const SurfaceIntersection &rec, const Vec3 &w_o, const Vec3 &w_i) {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local    = sFrame.toLocal(w_o);
    const auto w_i_local    = sFrame.toLocal(w_i);

    if (w_o_local.z == 0 || w_i_local.z == 0) return {};

    if (rec.material->mType == Material::DIFFUSE) {
        Vec3 albedo = rec.material->parameters.albedo;
        if (rec.material->textureIndices.albedo != -1) {
            albedo = scene.textures[rec.material->textureIndices.albedo].getTexel(rec.uv);
            albedo = sRGBToLinear(albedo);
        }

        const auto bxdf = DiffuseBxDF{albedo};
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    if (rec.material->mType == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior, rec.material->parameters.k};
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    if (rec.material->mType == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior.x);
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    return {};
}

float pdfBxdf(const Scene &scene, const Material *mat, const SurfaceIntersection &rec, const Vec3 &w_o, const Vec3 &w_i) {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local    = sFrame.toLocal(w_o);
    const auto w_i_local    = sFrame.toLocal(w_i);

    if (w_o_local.z == 0 || w_i_local.z == 0) return 0;

    if (rec.material->mType == Material::DIFFUSE) {
        const auto bxdf = DiffuseBxDF{rec.material->parameters.albedo};
        return bxdf.pdf(w_o_local, w_i_local);
    }

    if (rec.material->mType == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior, rec.material->parameters.k};
        return bxdf.pdf(w_o_local, w_i_local);
    }

    if (rec.material->mType == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->parameters.alphaX, rec.material->parameters.alphaY}, rec.material->parameters.ior.x);
        return bxdf.pdf(w_o_local, w_i_local);
    }

    return 0;
}
