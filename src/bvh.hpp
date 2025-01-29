#pragma once

#include "primitives.hpp"
#include "rt.hpp"

struct alignas(32) LinearBVHNode {
    AABB bbox;
    union {
        int primitivesOffset;
        int secondChildOffset;
    };
    uint16_t numPrimitives;
    uint8_t axis;
};

struct BVHNode {
    AABB bbox;
    BVHNode *children[2];
    int splitAxis;
    int firstPrimOffset;
    int numPrimitives;

    HOST void initLeaf(const int first, const int n, const AABB &bounds) {
        firstPrimOffset = first;
        numPrimitives = n;
        bbox = bounds;
        children[0] = children[1] = nullptr;
    }

    HOST void initBranch(const int axis, BVHNode *child0, BVHNode *child1) {
        children[0] = child0;
        children[1] = child1;
        bbox = AABB(child0->bbox, child1->bbox);
        splitAxis = axis;
        numPrimitives = 0;
    }

    HOSTDEV [[nodiscard]] bool isLeaf() const {
        return children[0] == nullptr && children[1] == nullptr;
    }

    HOSTDEV bool isBranch() const {
        return !isLeaf();
    }

    HOST void destroy() const {
        if (isBranch()) {
            children[0]->destroy();
            children[1]->destroy();

            delete children[0];
            delete children[1];
        }
    }
};

BVHNode *buildTree(std::span<Primitive> bvhPrimitives, int *totalNodes, int *orderedPrimitiveOffset, std::vector<Primitive> &orderedPrimitives, int maxPrimsInNode);

int flattenBVH(const BVHNode *node, LinearBVHNode *nodes, int *offset);
