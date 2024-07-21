#pragma once

#include "hittable.hpp"
#include "rtx.hpp"
#include <vector>


class HittableList : public Hittable {
public:
    std::vector<shared_ptr<Hittable>> objects;

    HittableList() = default;
    explicit HittableList(const shared_ptr<Hittable> &object) { add(object); }

    void clear() { objects.clear(); }

    void add(const shared_ptr<Hittable> &object) {
        objects.push_back(object);
    }

    bool hit(const Rayd &ray, const Interval &t, HitRecord &rec) const override {
        HitRecord tempRec;
        bool hitAnything = false;
        auto closest     = t.max;

        for (const auto &obj: objects) {
            if (obj->hit(ray, Interval(t.min, closest), tempRec)) {
                hitAnything = true;
                closest     = tempRec.t;
                rec         = tempRec;
            }
        }

        return hitAnything;
    }
};
