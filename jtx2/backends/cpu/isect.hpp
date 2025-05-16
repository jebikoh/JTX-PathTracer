#pragma once

#include <scene/scene.hpp>

namespace jtx {

struct SurfaceIntersection {
    vec3 point;
    vec3 normal;
    vec2 texCoords;
    vec2 uv;
    float t;
};

/**
 * Calculates closest hit of a single triangle in a scene. Interpolates normals and UVs.
 * @param scene scene containing the triangle
 * @param index index of the triangle to test
 * @param r ray
 * @param t0 minimum t
 * @param t1 maximum t
 * @param isect intersection information, will be populated if ray intersects triangle
 * @return true if ray intersects triangle, false otherwise
 */
inline bool tClosestHit(const Scene &scene, const int index, const ray &r, float t0, float t1, SurfaceIntersection &isect) {
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

    // Experiment
     //isect.uv = vec2(b1, b2); // V2

    // V1
    isect.point = r.at(root);

    float b0 = 1 - b1 - b2;

    vec3 n = b0 * scene.normals[tri.x] + b1 * scene.normals[tri.y] + b2 * scene.normals[tri.z];
    isect.normal = r.dir.dot(n) < 0 ? n : -n;

    isect.texCoords = scene.texCoords[tri.x] * b0 + scene.texCoords[tri.y] * b1 + scene.texCoords[tri.z] * b2;

    return true;
}

}// namespace jtx