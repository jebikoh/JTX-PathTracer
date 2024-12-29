#pragma once

#include "interval.hpp"
#include "../rt.hpp"

// TO-DO: Replace with version from PBRTv4
class AABB {
public:
    Vec3 pmin, pmax;

    AABB() {
        float minNum = std::numeric_limits<float>::lowest();
        float maxNum = std::numeric_limits<float>::max();
        pmin = {maxNum, maxNum, maxNum};
        pmax = {minNum, minNum, minNum};
    }

    AABB(const Vec3 &a, const Vec3 &b) {
        pmin = jtx::min(a, b);
        pmax = jtx::max(a, b);
    }

    AABB(const Interval &x, const Interval &y, const Interval &z) {
        pmin = {x.min, y.min, z.min};
        pmax = {x.max, y.max, z.max};
    }

    AABB(const AABB &a, const AABB &b) {
        pmin = jtx::min(a.pmin, b.pmin);
        pmax = jtx::max(a.pmax, b.pmax);
    }

    void expand(const AABB &other) {
        pmin = jtx::min(pmin, other.pmin);
        pmax = jtx::max(pmax, other.pmax);
    }

    void expand(const Vec3 &p) {
        pmin = jtx::min(pmin, p);
        pmax = jtx::max(pmax, p);
    }

    [[nodiscard]] Interval axis(const int i) const {
        if (i == 0) return {pmin.x, pmax.x};
        if (i == 1) return {pmin.y, pmax.y};
        return {pmin.z, pmax.z};
    }

    [[nodiscard]] int longestAxis() const {
        const Vec3 d = diagonal();
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }

    Vec3 offset(Vec3 p) const {
        Vec3 o = p - pmin;
        if (pmax.x > pmin.x) o.x /= pmax.x - pmin.x;
        if (pmax.y > pmin.y) o.y /= pmax.y - pmin.y;
        if (pmax.z > pmin.z) o.z /= pmax.z - pmin.z;
        return o;
    }

    [[nodiscard]] bool hit(const Vec3 &o, const Vec3 &d, const Interval &t) const {
        auto t0 = t.min;
        auto t1 = t.max;

        for (int i = 0; i < 3; ++i) {
            const auto invDir = 1 / d[i];
            auto tNear        = (pmin[i] - o[i]) * invDir;
            auto tFar         = (pmax[i] - o[i]) * invDir;

            if (tNear > tFar) jstd::swap(tNear, tFar);
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        return true;
    }

    Vec3 diagonal() const {
        return pmax - pmin;
    }

    float surfaceArea() const {
        const Vec3 d = diagonal();
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

    float volume() const {
        const Vec3 d = diagonal();
        return d.x * d.y * d.z;
    }

    static const AABB EMPTY;
    static const AABB UNIVERSE;
};