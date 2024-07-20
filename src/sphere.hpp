#pragma once
#include "hittable.hpp"
#include "rtx.hpp"

class Sphere : public Hittable {
public:
    Sphere(const Point3d &center,
           double radius,
           shared_ptr<Material> mat)
        : center(center),
          radius(fmax(0, radius)),
          mat(mat){};

    bool hit(const Rayd &ray, double tmin, double tmax, HitRecord &rec) const override {
        Vec3d oc = center - ray.origin;
        auto a   = ray.dir.lenSqr();
        auto h   = dot(ray.dir, oc);
        auto c   = oc.lenSqr() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0) return false;

        auto sqrtd = sqrt(discriminant);

        auto root = (h - sqrtd) / a;
        if (root <= tmin || root >= tmax) {
            root = (h + sqrtd) / a;
            if (root <= tmin || root >= tmax) {
                return false;
            }
        }

        rec.t = root;
        rec.p = ray.at(rec.t);

        // Flips normal if the ray is inside the sphere
        Vec3d outwardNormal = (rec.p - center) / radius;
        rec.setFaceNormal(ray, outwardNormal);
        return true;
    }

    bool hit(const Rayd &ray, const Interval &t, HitRecord &rec) const override {
        Vec3d oc = center - ray.origin;
        auto a   = ray.dir.lenSqr();
        auto h   = dot(ray.dir, oc);
        auto c   = oc.lenSqr() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0) return false;

        auto sqrtd = sqrt(discriminant);

        auto root = (h - sqrtd) / a;
        if (t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (t.surrounds(root)) {
                return false;
            }
        }

        rec.t   = root;
        rec.p   = ray.at(rec.t);
        rec.mat = mat;

        // Flips normal if the ray is inside the sphere
        Vec3d outwardNormal = (rec.p - center) / radius;
        rec.setFaceNormal(ray, outwardNormal);
        return true;
    }


private:
    Point3d center;
    double radius;
    shared_ptr<Material> mat;
};