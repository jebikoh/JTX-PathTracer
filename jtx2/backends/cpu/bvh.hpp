#pragma once

#include "isect.hpp"
#include "jtx.hpp"
#include "util/aabb.hpp"
#include "util/simd.hpp"

#include <scene/scene.hpp>

namespace jtx {

struct Triangle;

struct alignas(32) BVH2Node {
    AABB bbox;
    union {
        int trianglesOffset;
        int secondChildOffset;
    };
    uint16_t numTriangles;
    uint8_t axis;
};

struct BVH2BuildNode {
    AABB bbox;
    BVH2BuildNode *children[2];

    int splitAxis;
    int firstTriangleOffset;
    int numTriangles;

    void initLeaf(const int first, const int n, const AABB &bounds) {
        firstTriangleOffset = first;
        numTriangles        = n;
        bbox                = bounds;
        children[0] = children[1] = nullptr;
    }

    void initBranch(const int axis, BVH2BuildNode *left, BVH2BuildNode *right) {
        children[0]  = left;
        children[1]  = right;
        bbox         = AABB(left->bbox, right->bbox);
        splitAxis    = axis;
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
    BVH2Node *m_nodes   = nullptr;
    const Scene *m_scene = nullptr;
};

BVH2BuildNode *buildTree(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles, int maxTrianglesInNode);
int flattenBVH2(const BVH2BuildNode *node, BVH2Node *nodes, int *offset);

// BVH4/QBVH
static constexpr int32_t JTX_INT_MIN                     = 0x80000000;
static constexpr uint32_t JTX_BVH4_FIRST_INDEX_BIT_WIDTH = 27;
static constexpr uint32_t JTX_BVH4_FIRST_INDEX_MASK      = 0b00000111111111111111111111111111;
static constexpr uint32_t JTX_BVH4_TRI_COUNT_MASK        = 0b01111000000000000000000000000000;

struct alignas (128) BVH4Node {
    AABB4 bbox;
    /*
     * Children are encoded into a single 32-bit signed integer index:
     *
     * Child bit                   First triangle offset
     *   │                                   │
     *   ▼                                   ▼
     * ┌───┬───────┬──────────────────────────────────────────────────────┐
     * │ 1 │   4   │                         27                           │
     * └───┴───────┴──────────────────────────────────────────────────────┘
     *         ▲
     *         │
     *    # of triangles
     *
     * If the index is <0, then the child is leaf. If index >=0, then the child is an inner node.
     *
     * 4 bits are dedicated to encoding the number of triangles in a leaf. Since triangles are
     * processed in batches of four, we can denote up to 64 triangles in a single leaf node.
     *
     * The remaining 27 bits are used to encode the first triangle offset.
     */
    int32_t children[4];
    int32_t axis[3];
    int32_t fill;

    /**
     * Checks if the child at the given index is a leaf.
     * @param child child index [0, 4)
     * @return true if leaf, false otherwise
     */
    bool isLeaf(const size_t child) const {
        return children[child] < 0;
    }

    /**
     * Checks if the child at the given index is an inner node.
     * @param child child index [0, 4)
     * @return true if inner node, false otherwise
     */
    bool isInner(const size_t child) const {
        return children[child] >= 0;
    }

    /**
     * Retrieves the number of triangles in a leaf node.
     * Does verify if the child is a leaf node.
     * @param child child index [0, 4)
     * @return number of triangles in leaf
     */
    uint32_t getNumTriangles(const size_t child) const {
        return (((children[child] & JTX_BVH4_TRI_COUNT_MASK) >> JTX_BVH4_FIRST_INDEX_BIT_WIDTH) + 1) * 4;
    }

    /**
     * Retrieves the first triangle index in a leaf node.
     * Does not verify if teh child is a leaf node.
     * @param child child index [0, 4)
     * @return first triangle index
     */
    uint32_t getFirstTriangle(const size_t child) const {
        return children[child] & JTX_BVH4_FIRST_INDEX_MASK;
    }
};

BVH2BuildNode *buildTreeForBVH4(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles);

class BVH4 {
public:
    void build(const jtx::Scene &scene);
    void destroy();

    bool closestHit(const ray &r, float t0, float t1, SurfaceIntersection &isect) const;
    bool anyHit(const ray &r, float t0, float t1);

private:
    std::vector<Triangle> m_triangles;
    BVH4Node *m_nodes   = nullptr;
    const Scene *m_scene = nullptr;
};

}// namespace jtx