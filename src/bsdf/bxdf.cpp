#include "bxdf.hpp"
#include "../material.hpp"
#include "../scene.hpp"
#include "conductor.hpp"
#include "dielectric.hpp"
#include "diffuse.hpp"
#include "gltf.hpp"

bool sampleBxdf(const Scene &scene, const SurfaceIntersection &rec, const Vec3 &w_o, const float uc, const Vec2f &u, BSDFSample &s) {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local    = sFrame.toLocal(w_o);
    if (w_o_local.z == 0) return false;

    if (rec.material->type == Material::METALLIC_ROUGHNESS) {
        Vec3 albedo = rec.material->albedo;
        if (rec.material->albedoTexId != -1) {
            albedo = scene.textures[rec.material->albedoTexId].getTexel(rec.uv);
            albedo = sRGBToLinear(albedo);
        }

        float metallic  = rec.material->alphaX;
        float roughness = rec.material->alphaY;
        if (rec.material->metallicRoughnessTexId != -1) {
            auto mr   = scene.textures[rec.material->metallicRoughnessTexId].getTexel(rec.uv);
            roughness = mr.y;
            metallic  = mr.z;
        }

        const float roughness2 = roughness * roughness;
        const auto bxdf        = MetallicRoughnessBxDF{roughness2, albedo, metallic};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (rec.material->type == Material::DIFFUSE) {
        // If material has a diffuse texture, we should use that
        Vec3 albedo = rec.material->albedo;
        if (rec.material->albedoTexId != -1) {
            albedo = scene.textures[rec.material->albedoTexId].getTexel(rec.uv);
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

    if (rec.material->type == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->alphaX, rec.material->alphaY}, rec.material->IOR, rec.material->k};
        if (bxdf.sample(w_o_local, uc, u, s)) {
            if (!s.fSample || s.pdf == 0 || s.w_i.z == 0) return false;
            s.w_i = sFrame.toWorld(s.w_i);
            return true;
        }
        return false;
    }

    if (rec.material->type == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->alphaX, rec.material->alphaY}, rec.material->IOR.x);
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

    if (rec.material->type == Material::METALLIC_ROUGHNESS) {
        Vec3 albedo = rec.material->albedo;
        if (rec.material->albedoTexId != -1) {
            albedo = scene.textures[rec.material->albedoTexId].getTexel(rec.uv);
            albedo = sRGBToLinear(albedo);
        }

        float metallic  = rec.material->alphaX;
        float roughness = rec.material->alphaY;
        if (rec.material->metallicRoughnessTexId != -1) {
            auto mr   = scene.textures[rec.material->metallicRoughnessTexId].getTexel(rec.uv);
            roughness = mr.y;
            metallic  = mr.z;
        }

        const float roughness2 = roughness * roughness;
        const auto bxdf        = MetallicRoughnessBxDF{roughness2, albedo, metallic};
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::DIFFUSE) {
        Vec3 albedo = rec.material->albedo;
        if (rec.material->albedoTexId != -1) {
            albedo = scene.textures[rec.material->albedoTexId].getTexel(rec.uv);
            albedo = sRGBToLinear(albedo);
        }

        const auto bxdf = DiffuseBxDF{albedo};
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->alphaX, rec.material->alphaY}, rec.material->IOR, rec.material->k};
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->alphaX, rec.material->alphaY}, rec.material->IOR.x);
        return bxdf.evaluate(w_o_local, w_i_local);
    }

    return {};
}

float pdfBxdf(const Scene &scene, const Material *mat, const SurfaceIntersection &rec, const Vec3 &w_o, const Vec3 &w_i) {
    const jtx::Frame sFrame = jtx::Frame::fromZ(rec.normal);
    const auto w_o_local    = sFrame.toLocal(w_o);
    const auto w_i_local    = sFrame.toLocal(w_i);

    if (w_o_local.z == 0 || w_i_local.z == 0) return 0;

    if (rec.material->type == Material::METALLIC_ROUGHNESS) {
        float metallic  = rec.material->alphaX;
        float roughness = rec.material->alphaY;
        if (rec.material->metallicRoughnessTexId != -1) {
            auto mr   = scene.textures[rec.material->metallicRoughnessTexId].getTexel(rec.uv);
            roughness = mr.y;
            metallic  = mr.z;
        }

        const auto bxdf = MetallicRoughnessBxDF{roughness * roughness, rec.material->albedo, metallic};
        return bxdf.pdf(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::DIFFUSE) {
        const auto bxdf = DiffuseBxDF{rec.material->albedo};
        return bxdf.pdf(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::CONDUCTOR) {
        const auto bxdf = ConductorBxDF{{rec.material->alphaX, rec.material->alphaY}, rec.material->IOR, rec.material->k};
        return bxdf.pdf(w_o_local, w_i_local);
    }

    if (rec.material->type == Material::DIELECTRIC) {
        const auto bxdf = DielectricBxDF({rec.material->alphaX, rec.material->alphaY}, rec.material->IOR.x);
        return bxdf.pdf(w_o_local, w_i_local);
    }

    return 0;
}
