#pragma once

#include <limits>

#include "jtx.hpp"

namespace jtx {

static constexpr float JTX_FLOAT_MIN = std::numeric_limits<float>::lowest();
static constexpr float JTX_FLOAT_MAX = std::numeric_limits<float>::max();

enum Axis {
    JTX_AXIS_X = 0,
    JTX_AXIS_Y = 1,
    JTX_AXIS_Z = 2,
};

struct AABB {
    vec3 pmin, pmax;

    /**
     * Constructs an empty AABB with maximum bounds
     */
    AABB() {
        pmin = {JTX_FLOAT_MAX, JTX_FLOAT_MAX, JTX_FLOAT_MAX};
        pmax = {JTX_FLOAT_MAX, JTX_FLOAT_MAX, JTX_FLOAT_MAX};
    }

    /**
     * Constructs an AABB with the min/max of the given points
     * @param a first point
     * @param b second point
     */
    AABB(const vec3 &a, const vec3 &b) {
        pmin = jtx::min(a, b);
        pmax = jtx::max(a, b);
    }

    /**
     * Constructs an AABB of the union of two AABBs
     * @param a first AABB
     * @param b second AABB
     */
    AABB(const AABB &a, const AABB &b) {
        pmin = jtx::min(a.pmin, b.pmin);
        pmax = jtx::max(a.pmax, b.pmax);
    }

    AABB(const vec3 &a, const vec3 &b, const vec3 &c) {
        pmin = jtx::min(jtx::min(a, b), c);
        pmax = jtx::max(jtx::max(a, b), c);
    }

    /**
     * Expands this AABB to include the given AABB
     * @param other AABB to include
     */
    void expand(const AABB &other) {
        pmin = jtx::min(pmin, other.pmin);
        pmax = jtx::max(pmax, other.pmax);
    }

    /**
     * Expands this AABB to include the given point
     * @param p point to include
     */
    void expand(const vec3 &p) {
        pmin = jtx::min(pmin, p);
        pmax = jtx::max(pmax, p);
    }

    /**
     * Retrieves the min/max of the AABB on the given axis
     * @param axis axis to retrieve
     * @return vec2(min, max) of the AABB on the given axis
     */
    vec2 axis(const Axis axis) const {
        switch (axis) {
            case JTX_AXIS_X:
                return {pmin.x, pmax.x};
            case JTX_AXIS_Y:
                return {pmin.y, pmax.y};
            case JTX_AXIS_Z:
            default:
                return {pmin.z, pmax.z};
        }
    }

    /**
     * Calculates the diagonal of the AABB (un-normalized)
     * @return vec3 diagonal
     */
    vec3 diagonal() const { return pmax - pmin; }

    /**
     * Calculates the surface area of this AABB
     * @return surface area
     */
    float surfaceArea() const {
        const vec3 diag = diagonal();
        return 2 * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
    }

    /**
     * Calculates the volume of this AABB
     * @return volume
     */
    float volume() const {
        const vec3 diag = diagonal();
        return diag.x * diag.y * diag.z;
    }

    /**
     * Calculates the longest axis of this AABB
     * @return longest axis
     */
    Axis longestAxis() const {
        const vec3 diag = diagonal();
        if (diag.x > diag.y && diag.x > diag.z) return JTX_AXIS_X;
        if (diag.y > diag.z) return JTX_AXIS_Y;
        return JTX_AXIS_Z;
    }

    /**
     * Calculates the offset of a point relative to the corners of this AABB, where
     * (0, 0, 0) is the minimum corner and (1, 1, 1) is the maximum corner
     *
     * @param p point to calculate offset
     * @return relative offset
     */
    vec3 offset(const vec3 &p) const {
        vec3 o = p - pmin;
        if (pmax.x > pmin.x) o.x /= pmax.x - pmin.x;
        if (pmax.y > pmin.y) o.y /= pmax.y - pmin.y;
        if (pmax.z > pmin.z) o.z /= pmax.z - pmin.z;
        return o;
    }

    /**
     * Checks if a ray with origin o and direction d intersects this AABB between t0 and t1
     * @param o ray origin
     * @param d ray direction
     * @param t0 minimum t value
     * @param t1 maximum t value
     * @return true if the ray intersects the AABB, false otherwise
     */
    bool hit(const vec3 &o, const vec3 &d, float t0, float t1) const {
        for (int i = 0; i < 3; ++i) {
            const auto invDir = 1 / d[i];
            auto tNear        = (pmin[i] - o[i]) * invDir;
            auto tFar         = (pmax[i] - o[i]) * invDir;

            if (tNear > tFar) std::swap(tNear, tFar);
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        return true;
    }
};

}// namespace jtx