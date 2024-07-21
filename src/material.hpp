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
    explicit Metal(const Color &albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const Rayd &in,
                 const HitRecord &rec,
                 Color &attenuation,
                 Rayd &scattered) const override {
        Vec3d reflected = jtx::reflect(in.dir, rec.normal).normalize();
        reflected += (fuzz * randomUnitVector());
        scattered   = Rayd(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.dir, rec.normal) > 0);
    }

private:
    Color albedo;
    double fuzz;
};

class Dielectric : public Material {
public:
    explicit Dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const Rayd &in,
                 const HitRecord &rec,
                 Color &attenuation,
                 Rayd &scattered) const override {
        attenuation = Color(1.0, 1.0, 1.0);
        double ri   = rec.frontFace ? (1.0 / refraction_index) : refraction_index;

        auto unitDir    = normalize(in.dir);
        double cosTheta = fmin(dot(-unitDir, rec.normal), 1.0);
        double sinTheta = sqrt(1.0 - cosTheta * cosTheta);

        Vec3d direction;

        if ((ri * sinTheta > 1.0) || reflectance(cosTheta, ri) > jtx::random<double>()) {
            direction = jtx::reflect(unitDir, rec.normal);
        } else {
            direction = jtx::refract(unitDir, rec.normal, ri);
        }

        scattered = Rayd(rec.p, direction);
        return true;
    }

private:
    double refraction_index;

    static double reflectance(double cosine, double ri) {
        auto r0 = (1 - ri) / (1 + ri);
        r0      = r0 * r0;
        return r0 + (1 - r0) * std::pow((1 - cosine), 5);
    }
};