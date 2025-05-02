#pragma once

#include "jtx.hpp"
#include "util/aabb.hpp"

namespace jtx {
struct Triangle;

struct alignas(32) LBVH2Node {
    AABB bbox;
    union {
        int primitivesOffset;
        int secondChildOffset;
    };
    uint16_t numPrimitives;
    uint8_t axis;
};

struct BVH2Node {
    AABB bbox;
    BVH2Node *children[2];

    int splitAxis;
    int firstPrimitiveOffset;
    int numPrimitives;

    void initLeaf(const int first, const int n, const AABB &bounds) {
        firstPrimitiveOffset = first;
        numPrimitives        = n;
        bbox                 = bounds;
        children[0] = children[1] = nullptr;
    }

    void initBranch(const int axis, BVH2Node *left, BVH2Node *right) {
        children[0]   = left;
        children[1]   = right;
        bbox          = AABB(left->bbox, right->bbox);
        splitAxis     = axis;
        numPrimitives = 0;
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

BVH2Node *buildTree(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles, int maxTrianglesInNode);

}// namespace jtx