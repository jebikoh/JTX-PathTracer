#pragma once

#include <scene/scene.hpp>

namespace jtx {

struct TriangleIntersection {
    float t, u, v;
    uint32_t index;
};

/**
 * Calculates a ray-triangle intersection
 * @param scene scene containing the triangle
 * @param index index of the triangle to test
 * @param r ray to test
 * @param t0 lower bound of t
 * @param t1 upper bound of t
 * @param isect triangle intersection data. Will be populated if intersection occurs
 * @return true if intersection occurs, false otherwise
 */
JTX_FORCE_INLINE bool triangleHit(const Scene &scene, const int index, const ray &r, float t0, float t1, TriangleIntersection &isect) {
    const vec3u tri = scene.indices[index];
    vec3 v0         = scene.positions[tri.x];
    vec3 v1         = scene.positions[tri.y];
    vec3 v2         = scene.positions[tri.z];

    const vec3 v0v1 = v1 - v0;
    const vec3 v0v2 = v2 - v0;
    const vec3 pvec = jtx::cross(r.dir, v0v2);
    const float det  = v0v1.dot(pvec);

    if (fabs(det) < 1e-8) return false;

    const float invDet = 1.0f / det;
    const vec3 tvec    = r.origin - v0;

    const float b1 = tvec.dot(pvec) * invDet;
    if (b1 < 0 || b1 > 1) return false;

    const vec3 qvec = jtx::cross(tvec, v0v1);
    const float b2               = r.dir.dot(qvec) * invDet;
    if (b2 < 0 || b1 + b2 > 1) return false;

    const float root = v0v2.dot(qvec) * invDet;
    if (!(t0 < root && root < t1)) return false;

    isect.t = root;
    isect.u = b1;
    isect.v = b2;
    isect.index = index;

    return true;
}

/**
 * Calculates a ray-triangle intersection
 * @param scene scene containing the triangle
 * @param index index of the triangle to test
 * @param r ray to test
 * @param t0 lower bound of t
 * @param t1 upper bound of t
 * @return true if intersection occurs, false otherwise
 */
JTX_FORCE_INLINE bool triangleOccluded(const Scene &scene, const int index, const ray &r, float t0, float t1) {
    const vec3u tri = scene.indices[index];
    vec3 v0         = scene.positions[tri.x];
    vec3 v1         = scene.positions[tri.y];
    vec3 v2         = scene.positions[tri.z];

    const vec3 v0v1 = v1 - v0;
    const vec3 v0v2 = v2 - v0;
    const vec3 pvec = jtx::cross(r.dir, v0v2);
    const float det  = v0v1.dot(pvec);

    if (fabs(det) < 1e-8) return false;

    const float invDet = 1.0f / det;
    const vec3 tvec    = r.origin - v0;

    const float b1 = tvec.dot(pvec) * invDet;
    if (b1 < 0 || b1 > 1) return false;

    const vec3 qvec = jtx::cross(tvec, v0v1);
    const float b2               = r.dir.dot(qvec) * invDet;
    if (b2 < 0 || b1 + b2 > 1) return false;

    const float root = v0v2.dot(qvec) * invDet;
    if (!(t0 < root && root < t1)) return false;

    return true;
}

}// namespace jtx