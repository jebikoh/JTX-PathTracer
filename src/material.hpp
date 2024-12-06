#pragma once

#include "hittable.hpp"
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

        scattered = Ray(record.point, scatterDir);
        attenuation = _albedo;
        return true;
    }
private:
    Color _albedo;
};

class Metal {
public:
    Metal(const Color &albedo, Float fuzz) : _albedo(albedo), _fuzz(fuzz) {}

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
        Vec3 reflected = jtx::reflect(r.dir, record.normal).normalize() + _fuzz * randomUnitVector();
        scattered = Ray(record.point, reflected);
        attenuation = _albedo;
        return (jtx::dot(scattered.dir, record.normal) > 0);
    }
private:
    Color _albedo;
    Float _fuzz;
};

class Dielectric {
public:
    explicit Dielectric(Float refractionIndex) : _refractionIndex(refractionIndex) {}

    bool scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
        attenuation = Color(1.0, 1.0, 1.0);
        const Float ri =  record.frontFace ? (1.0 / _refractionIndex) : _refractionIndex;
        scattered = Ray(record.point, jtx::refract(jtx::normalize(r.dir), record.normal, ri));
        return true;
    }
private:
    Float _refractionIndex;
};

bool Material::scatter(const Ray &r, const HitRecord &record, Color &attenuation, Ray &scattered) const {
    auto fn = [&](auto ptr) { return ptr->scatter(r, record, attenuation, scattered); };
    return dispatch(fn);
}