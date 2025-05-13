#pragma once

#include <limits>

#include "jtx.hpp"
#include "simd.hpp"

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
        pmax = {JTX_FLOAT_MIN, JTX_FLOAT_MIN, JTX_FLOAT_MIN};
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
     * @param r ray
     * @param invDir inverse direction
     * @param t0 minimum t value
     * @param t1 maximum t value
     * @return true if the ray intersects the AABB, false otherwise
     */
    bool hit(const ray &r, const vec3 &invDir, float t0, float t1) const {
        for (int i = 0; i < 3; ++i) {
            const float d = invDir[i];
            auto tNear         = (pmin[i] - r.origin[i]) * d;
            auto tFar          = (pmax[i] - r.origin[i]) * d;

            if (tNear > tFar) std::swap(tNear, tFar);
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        return true;
    }
};

struct AABB4 {
    // X Y Z
    union {
        struct {
            vfloat4 pmin[3];
            vfloat4 pmax[3];
        };
        vfloat4 corners[2][3];
    };

    union HitResult {
        bool bHit[4];
        uint32_t val;
    };

    struct RayHitInfo {
        vec3 invDir;
        int sign[3];
    };

    // https://people.csail.mit.edu/amy/papers/box-jgt.pdf
    HitResult hit(const ray &r, const RayHitInfo &info, const float t0, const float t1) const {
        HitResult result;
#ifdef JTX_SIMD_X86_SSE4_2
        __m128 tmin = _mm_set1_ps(t0);
        __m128 tmax = _mm_set1_ps(t1);

        __m128 origin_x = _mm_set1_ps(r.origin.x);
        __m128 origin_y = _mm_set1_ps(r.origin.y);
        __m128 origin_z = _mm_set1_ps(r.origin.z);

        __m128 invDir_x = _mm_set1_ps(info.invDir.x);
        __m128 invDir_y = _mm_set1_ps(info.invDir.y);
        __m128 invDir_z = _mm_set1_ps(info.invDir.z);

        tmin = _mm_max_ps(_mm_mul_ps(_mm_sub_ps(info.sign[0] ? pmax[0].v4 : pmin[0].v4, origin_x), invDir_x), tmin);
        tmax = _mm_min_ps(_mm_mul_ps(_mm_sub_ps(info.sign[0] ? pmin[0].v4 : pmax[0].v4, origin_x), invDir_x), tmax);
        tmin = _mm_max_ps(_mm_mul_ps(_mm_sub_ps(info.sign[1] ? pmax[1].v4 : pmin[1].v4, origin_y), invDir_y), tmin);
        tmax = _mm_min_ps(_mm_mul_ps(_mm_sub_ps(info.sign[1] ? pmin[1].v4 : pmax[1].v4, origin_y), invDir_y), tmax);
        tmin = _mm_max_ps(_mm_mul_ps(_mm_sub_ps(info.sign[2] ? pmax[2].v4 : pmin[2].v4, origin_z), invDir_z), tmin);
        tmax = _mm_min_ps(_mm_mul_ps(_mm_sub_ps(info.sign[2] ? pmin[2].v4 : pmax[2].v4, origin_z), invDir_z), tmax);

        const auto mask = _mm_movemask_ps(_mm_cmple_ps(tmin, tmax));
        result.bHit[0] = (mask & 0b0001) != 0;
        result.bHit[1] = (mask & 0b0010) != 0;
        result.bHit[2] = (mask & 0b0100) != 0;
        result.bHit[3] = (mask & 0b1000) != 0;
#elif defined JTX_SIMD_ARM_NEON
        float32x4_t tmin = vdupq_n_f32(t0);
        float32x4_t tmax = vdupq_n_f32(t1);

        const float32x4_t origin_x = vdupq_n_f32(r.origin.x);
        const float32x4_t origin_y = vdupq_n_f32(r.origin.y);
        const float32x4_t origin_z = vdupq_n_f32(r.origin.z);
        const float32x4_t invDir_x = vdupq_n_f32(info.invDir.x);
        const float32x4_t invDir_y = vdupq_n_f32(info.invDir.y);
        const float32x4_t invDir_z = vdupq_n_f32(info.invDir.z);

        tmin = vmaxq_f32(vmulq_f32(vsubq_f32(info.sign[0] ? pmax[0].v4 : pmin[0].v4, origin_x), invDir_x), tmin);
        tmax = vminq_f32(vmulq_f32(vsubq_f32(info.sign[0] ? pmin[0].v4 : pmax[0].v4, origin_x), invDir_x), tmax);
        tmin = vmaxq_f32(vmulq_f32(vsubq_f32(info.sign[1] ? pmax[1].v4 : pmin[1].v4, origin_y), invDir_y), tmin);
        tmax = vminq_f32(vmulq_f32(vsubq_f32(info.sign[1] ? pmin[1].v4 : pmax[1].v4, origin_y), invDir_y), tmax);
        tmin = vmaxq_f32(vmulq_f32(vsubq_f32(info.sign[2] ? pmax[2].v4 : pmin[2].v4, origin_z), invDir_z), tmin);
        tmax = vminq_f32(vmulq_f32(vsubq_f32(info.sign[2] ? pmin[2].v4 : pmax[2].v4, origin_z), invDir_z), tmax);

        const uint32x4_t hitMask = vcleq_f32(tmin, tmax);
        result.bHit[0] = vgetq_lane_u32(hitMask, 0) != 0;
        result.bHit[1] = vgetq_lane_u32(hitMask, 1) != 0;
        result.bHit[2] = vgetq_lane_u32(hitMask, 2) != 0;
        result.bHit[3] = vgetq_lane_u32(hitMask, 3) != 0;
#else
        // Scalar version
        for (int i = 0; i < 4; ++i) {
            vec3 bounds[2];
            bounds[0] = {pmin[0].v[i], pmin[1].v[i], pmin[2].v[i]};
            bounds[1] = {pmax[0].v[i], pmax[1].v[i], pmax[2].v[i]};

            float tmin        = (bounds[info.sign[0]].x - r.origin.x) * info.invDir.x;
            float tmax        = (bounds[1 - info.sign[0]].x - r.origin.x) * info.invDir.x;
            const float tymin = (bounds[info.sign[1]].y - r.origin.y) * info.invDir.y;
            const float tymax = (bounds[1 - info.sign[1]].y - r.origin.y) * info.invDir.y;

            if ((tmin > tymax) || (tymin > tmax)) {
                result.bHit[i] = false;
                continue;
            }
            if (tymin > tmin) tmin = tymin;
            if (tymax < tmax) tmax = tymax;

            const float tzmin = (bounds[info.sign[2]].z - r.origin.z) * info.invDir.z;
            const float tzmax = (bounds[1 - info.sign[2]].z - r.origin.z) * info.invDir.z;

            if ((tmin > tzmax) || (tzmin > tmax)) {
                result.bHit[i] = false;
                continue;
            }
            if (tzmin > tmin) tmin = tzmin;
            if (tzmax < tmax) tmax = tzmax;

            result.bHit[i] = ((tmin <= t1) && (tmax >= t0));
        }
#endif
        return result;
    }
};

}// namespace jtx