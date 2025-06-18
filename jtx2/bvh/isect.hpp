#pragma once

#include <jtx.hpp>
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
JTX_FORCE_INLINE bool TriangleHit(const Scene &scene, const int index, const ray &r, float t0, float t1, TriangleIntersection &isect) {
    const vec3u tri = scene.indices[index];
    vec3 v0         = scene.positions[tri.x];
    vec3 v1         = scene.positions[tri.y];
    vec3 v2         = scene.positions[tri.z];

    const vec3 v0v1 = v1 - v0;
    const vec3 v0v2 = v2 - v0;
    const vec3 pv   = jtx::Cross(r.dir, v0v2);
    const float det = v0v1.Dot(pv);

    if (fabs(det) < 1e-8) return false;

    const float invDet = 1.0f / det;
    const vec3 tv      = r.origin - v0;

    const float b1 = tv.Dot(pv) * invDet;
    if (b1 < 0 || b1 > 1) return false;

    const vec3 qv  = jtx::Cross(tv, v0v1);
    const float b2 = r.dir.Dot(qv) * invDet;
    if (b2 < 0 || b1 + b2 > 1) return false;

    const float root = v0v2.Dot(qv) * invDet;
    if (!(t0 < root && root < t1)) return false;

    isect.t     = root;
    isect.u     = b1;
    isect.v     = b2;
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
JTX_FORCE_INLINE bool TriangleOccluded(const Scene &scene, const int index, const ray &r, float t0, float t1) {
    const vec3u tri = scene.indices[index];
    vec3 v0         = scene.positions[tri.x];
    vec3 v1         = scene.positions[tri.y];
    vec3 v2         = scene.positions[tri.z];

    const vec3 v0v1 = v1 - v0;
    const vec3 v0v2 = v2 - v0;
    const vec3 pv   = Cross(r.dir, v0v2);
    const float det = v0v1.Dot(pv);

    if (fabs(det) < 1e-8) return false;

    const float invDet = 1.0f / det;
    const vec3 tv      = r.origin - v0;

    const float b1 = tv.Dot(pv) * invDet;
    if (b1 < 0 || b1 > 1) return false;

    const vec3 qv  = jtx::Cross(tv, v0v1);
    const float b2 = r.dir.Dot(qv) * invDet;
    if (b2 < 0 || b1 + b2 > 1) return false;

    const float root = v0v2.Dot(qv) * invDet;
    if (!(t0 < root && root < t1)) return false;

    return true;
}

struct SurfaceAttributes {
    vec3 point;
    vec3 normal;
    vec2 texCoords;
    const Material *material;
};

JTX_FORCE_INLINE void InterpolateVertexAttributes(const Scene &scene, const ray &r, const TriangleIntersection &isect, SurfaceAttributes &surface) {
    const vec3u tri = scene.indices[isect.index];
    const vec3 n0   = scene.normals[tri.x];
    const vec3 n1   = scene.normals[tri.y];
    const vec3 n2   = scene.normals[tri.z];

    const float w = 1 - isect.u - isect.v;

    surface.point  = r.At(isect.t);
    surface.normal = n0 * w + n1 * isect.u + n2 * isect.v;

    const vec2 tex0 = scene.texCoords[tri.x];
    const vec2 tex1 = scene.texCoords[tri.y];
    const vec2 tex2 = scene.texCoords[tri.z];

    surface.texCoords = tex0 * w + tex1 * isect.u + tex2 * isect.v;
    surface.material  = &scene.materials[scene.materialIndices[isect.index]];
}

}// namespace jtx