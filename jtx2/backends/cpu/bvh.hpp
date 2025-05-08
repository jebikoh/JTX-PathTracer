#pragma once

#include "jtx.hpp"
#include "util/aabb.hpp"
#include "isect.hpp"

#include <scene/scene.hpp>

namespace jtx {

struct Triangle;

struct alignas(32) LBVH2Node {
    AABB bbox;
    union {
        int trianglesOffset;
        int secondChildOffset;
    };
    uint16_t numTriangles;
    uint8_t axis;
};

struct BVH2Node {
    AABB bbox;
    BVH2Node *children[2];

    int splitAxis;
    int firstTriangleOffset;
    int numTriangles;

    void initLeaf(const int first, const int n, const AABB &bounds) {
        firstTriangleOffset = first;
        numTriangles        = n;
        bbox                 = bounds;
        children[0] = children[1] = nullptr;
    }

    void initBranch(const int axis, BVH2Node *left, BVH2Node *right) {
        children[0]   = left;
        children[1]   = right;
        bbox          = AABB(left->bbox, right->bbox);
        splitAxis     = axis;
        numTriangles = 0;
    }

    bool isLeaf() const {
        return children[0] == nullptr && children[1] == nullptr;
    }

    bool isBranch() const {
        return !isLeaf();
    }

    void destroy() const {
        if (isBranch()) {
            children[0]->destroy();
            children[1]->destroy();

            delete children[0];
            delete children[1];
        }
    }
};

class BVH2 {
public:
    void build(const jtx::Scene &scene, int maxTrianglesInNode = 1);
    void destroy();

    bool closestHit(const ray &r, float t0, float t1, SurfaceIntersection &isect) const;
    bool anyHit(const ray &r, float t0, float t1);
private:
    int m_maxTrianglesInNode = 0;
    std::vector<Triangle> m_triangles;
    LBVH2Node *m_nodes = nullptr;
    const Scene *m_scene = nullptr;
};

BVH2Node *buildTree(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles, int maxTrianglesInNode);
int flattenBVH2(const BVH2Node *node, LBVH2Node *nodes, int *offset);

}// namespace jtx