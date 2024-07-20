#pragma once
#include "rtx.hpp"
#include <jtxlib/math/vecmath.hpp>

class Material {
public:
    virtual ~Material() = default;

    virtual bool scatter(
            const Rayd &in,
            const HitRecord &rec,
            Color &attenuation,
            Rayd &scattered) const {
        return false;
    }
};

class Lambertian : public Material {
public:
    explicit Lambertian(const Color &albedo) : albedo(albedo) {}

    bool scatter(const Rayd &in,
                 const HitRecord &rec,
                 Color &attenuation,
                 Rayd &scattered) const override {
        auto scatterDir = rec.normal + randomUnitVector();
        if (nearZero(scatterDir)) scatterDir = rec.normal;

        scattered   = Rayd(rec.p, scatterDir);
        attenuation = albedo;
        return true;
    }

private:
    Color albedo;
};

class Metal : public Material {
public:
    explicit Metal(const Color &albedo) : albedo(albedo) {}

    bool scatter(const Rayd &in,
                 const HitRecord &rec,
                 Color &attenuation,
                 Rayd &scattered) const override {
        scattered   = Rayd(rec.p, jtx::reflect(in.dir, rec.normal));
        attenuation = albedo;
        return true;
    }

private:
    Color albedo;
};