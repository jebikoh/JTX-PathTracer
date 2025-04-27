#pragma once

#include <limits>

#include "jtx.hpp"

namespace jtx {

static constexpr JTX_FLOAT_MIN = std::numeric_limits<float>::lowest();
static constexpr JTX_FLOAT_MAX = std::numeric_limits<float>::max();
struct AABB {
    enum Axis { X = 0,
                Y = 1,
                Z = 2 };

    vec3 pmin, pmax;

    AABB() {
        pmin = {JTX_FLOAT_MAX, JTX_FLOAT_MAX, JTX_FLOAT_MAX};
        pmax = {JTX_FLOAT_MAX, JTX_FLOAT_MAX, JTX_FLOAT_MAX};
    }

    AABB(const vec3 &a, const vec3 &b) {
        pmin = jtx::min(a, b);
        pmax = jtx::max(a, b);
    }

    AABB(const AABB &a, const AABB &b) {
        pmin = jtx::min(a.pmin, b.pmin);
        pmax = jtx::max(a.pmax, b.pmax);
    }

    void expand(const AABB &other) {
        pmin = jtx::min(pmin, other.pmin);
        pmax = jtx::max(pmax, other.pmax);
    }

    void expand(const vec3 &p) {
        pmin = jtx::min(pmin, p);
        pmax = jtx::max(pmax, p);
    }

    vec2 axis(const Axis axis) const {
        switch (axis) {
            case Axis::X:
                return {pmin.x, pmax.x};
            case Axis::Y:
                return {pmin.y, pmax.y};
            case Axis::Z:
            default:
                return {pmin.z, pmax.z};
        }
    }

    vec3 diagonal() const { return pmax - pmin; }

    vec3 surfaceArea() const {
        const vec3 diag = diagonal();
        return 2 * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
    }

    float volume() const {
        const vec3 diag = diagonal();
        return d.x * d.y * d.z;
    }

    Axis longestAxis() const {
        const vec3 diag = diagonal();
        if (d.x > d.y && d.x > d.z) return Axis::X;
        if (d.y > d.z) return Axis::Y;
        return Axis::Z;
    }

    vec3 offset(const vec3 &p) const {
        vec3 o = p - pmin;
        if (pmax.x > pmin.x) o.x /= pmax.x - pmin.x;
        if (pmax.y > pmin.y) o.y /= pmax.y - pmin.y;
        if (pmax.z > pmin.z) o.z /= pmax.z - pmin.z;
        return o;
    }

    bool hit(const vec3 &o, const vec3 &d, const float t0, const float t1) const {
        for (int i = 0; i < 3; ++i) {
            const auto invDir = 1 / d[i];
            const auto tNear  = (pmin[i] - o[i]) * invDir;
            const auto tFar   = (pmax[i] - o[i]) * invDir;

            if (tNear > tFar) std::swap(tNear, tFar);
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        return true;
    }
}

}// namespace jtx