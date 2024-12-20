#pragma once

#include "color.hpp"
#include "hit.hpp"
#include <jtxlib/util/taggedptr.hpp>

class Lambertian;
class Metal;
class Dielectric;

class Material : jtx::TaggedPtr<Lambertian, Metal, Dielectric> {
public:
    using TaggedPtr::TaggedPtr;

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const;
};

class Lambertian {
public:
    explicit Lambertian(const Color &albedo) : _albedo(albedo) {}

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
        auto scatterDir = record.normal + randomUnitVector();
        if (nearZero(scatterDir)) {
            scatterDir = record.normal;
        }

        scattered = Ray(record.point, scatterDir, r.time);
        attenuation = _albedo;
        return true;
    }
private:
    Color _albedo;
};

class Metal {
public:
    Metal(const Color &albedo, const Float fuzz) : _albedo(albedo), _fuzz(fuzz) {}

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
        Vec3 reflected = jtx::reflect(r.dir, record.normal).normalize() + _fuzz * randomUnitVector();
        scattered = Ray(record.point, reflected, r.time);
        attenuation = _albedo;
        return (jtx::dot(scattered.dir, record.normal) > 0);
    }
private:
    Color _albedo;
    Float _fuzz;
};

static Float reflectance(const Float cosine, const Float ri) {
    Float r0 = (1 - ri) / (1 + ri);
    r0       = r0 * r0;
    return r0 + (1 - r0) * jtx::pow((1 - cosine), 5);
}

class Dielectric {
public:
    explicit Dielectric(const Float refractionIndex) : _refractionIndex(refractionIndex) {}

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
        attenuation = Color(1.0, 1.0, 1.0);
        const Float ri = record.frontFace ? (1.0 / _refractionIndex) : _refractionIndex;

        const Vec3 dir = jtx::normalize(r.dir);
        const Float cosTheta = jtx::min(jtx::dot(-dir, record.normal), static_cast<Float>(1.0));
        const Float sinTheta = jtx::sqrt(1.0 - cosTheta * cosTheta);

        Vec3 scatterDir;
        if (ri * sinTheta > 1.0 || reflectance(cosTheta, ri) > randomFloat()) {
            scatterDir = jtx::reflect(dir, record.normal);
        } else {
            scatterDir = jtx::refract(dir, record.normal, ri);
        }

        scattered = Ray(record.point, scatterDir, r.time);
        return true;
    }
private:
    Float _refractionIndex;
};

inline bool Material::scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
    auto fn = [&](auto ptr) { return ptr->scatter(r, record, attenuation, scattered); };
    return dispatch(fn);
}