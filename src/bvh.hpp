#pragma once

#include "rt.hpp"
#include "util/interval.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "primitives.hpp"

class LinearBVHNode;

struct BVHNode {
    AABB bbox;
    BVHNode *children[2];
    int splitAxis;
    int firstPrimOffset;
    int numPrimitives;

    void initLeaf(const int first, const int n, const AABB &bounds) {
        firstPrimOffset = first;
        numPrimitives = n;
        bbox = bounds;
        children[0] = children[1] = nullptr;
    }

    void initBranch(const int axis, BVHNode *child0, BVHNode *child1) {
        children[0] = child0;
        children[1] = child1;
        bbox = AABB(child0->bbox, child1->bbox);
        splitAxis = axis;
        numPrimitives = 0;
    }
};

class BVHTree {
public:
    BVHTree(const Scene &scene, int maxPrimsInNode);
private:
    BVHNode *buildTree(std::span<Primitive> bvhPrimitives, int *totalNodes, int *orderedPrimitiveOffset, std::vector<Primitive> &orderedPrimitives);

    int maxPrimsInNode_;
    std::vector<Primitive> primitives_;
    const Scene &scene_;
    // LinearBVHNode *nodes_ = nullptr;
    BVHNode *root_;
};