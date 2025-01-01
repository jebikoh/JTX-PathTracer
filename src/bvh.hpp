#pragma once

#include "rt.hpp"
#include "scene.hpp"
#include "primitives.hpp"

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

class BVHTree {
public:
    BVHTree(const Scene &scene, int maxPrimsInNode);
    ~BVHTree() {
        if (nodes_) destroy();
    }

    void destroy() {
        delete[] nodes_;
        nodes_ = nullptr;
    }

    AABB bounds() const {
        return nodes_[0].bbox;
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record);

private:
    BVHNode *buildTree(std::span<Primitive> bvhPrimitives, int *totalNodes, int *orderedPrimitiveOffset, std::vector<Primitive> &orderedPrimitives);

    int flattenBVH(const BVHNode *node, int *offset);

    int maxPrimsInNode_;
    std::vector<Primitive> primitives_;
    const Scene &scene_;
    LinearBVHNode *nodes_ = nullptr;
};