#pragma once

#include "material.hpp"
#include "rt.hpp"
#include "util/aabb.hpp"

#include <complex>

struct Mesh {
    int numVertices;
    int numIndices;

    Vec3i *indices;
    Vec3 *vertices;
    Vec3 *normals;

    Material &material;

    Mesh(Vec3i *indices, const int numIndices, Vec3 *vertices, const int numVertices, Vec3 *normals, Material &material)
        : numVertices(numVertices),
          numIndices(numIndices),
          indices(indices),
          vertices(vertices),
          normals(normals),
          material(material) {}

    void getVertices(const int index, Vec3 &v0, Vec3 &v1, Vec3 &v2) const {
        const Vec3i i = indices[index];
        v0            = vertices[i[0]];
        v1            = vertices[i[1]];
        v2            = vertices[i[2]];
    }

    AABB tBounds(const int index) const {
        Vec3 v0, v1, v2;
        getVertices(index, v0, v1, v2);
        return AABB{v0, v1}.expand(v2);
    }

    float tArea(const int index) const {
        Vec3 v0, v1, v2;
        getVertices(index, v0, v1, v2);
        return 0.5f * jtx::cross(v1 - v0, v2 - v0).len();
    }

    void getNormals(const int index, Vec3 &n0, Vec3 &n1, Vec3 &n2) const {
        const Vec3i i = indices[index];
        n0            = normals[i[0]];
        n1            = normals[i[1]];
        n2            = normals[i[2]];
    }

    bool tHit(const Ray &r, const Interval t, HitRecord &record, const int index, float &u, float &v) const {
        Vec3 v0, v1, v2;
        getVertices(index, v0, v1, v2);
        const auto v0v1 = v1 - v0;
        const auto v0v2 = v2 - v0;
        const auto pvec = jtx::cross(r.dir, v0v2);
        const auto det  = v0v1.dot(pvec);

        if (fabs(det) < 1e-8) return false;

        const float invDet = 1 / det;
        const auto tvec    = r.origin - v0;

        u = tvec.dot(pvec) * invDet;
        if (u < 0 || u > 1) return false;

        const auto qvec = tvec.cross(v0v1);
        v               = r.dir.dot(qvec) * invDet;
        if (v < 0 || u + v > 1) return false;

        const float root = v0v2.dot(qvec) * invDet;
        if (!t.surrounds(root)) return false;

        record.t        = root;
        record.point    = r.at(root);
        record.material = &material;

        Vec3 n0, n1, n2;
        getNormals(index, n0, n1, n2);

        // Interpolate with barycentric
        const Vec3 n = (1 - u - v) * n0 + u * n1 + v * n2;
        record.setFaceNormal(r, n);

        return true;
    }

    void destroy() const {
        if (indices) delete[] indices;
        if (vertices) delete[] vertices;
        if (normals) delete[] normals;
    }
};

struct Triangle {
    int index;
    int meshIndex;
};