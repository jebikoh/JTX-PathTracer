#include "bvh.hpp"

struct BVHBucket {
    int count = 0;
    AABB bounds;
};


BVHNode *buildTree(std::span<Primitive> bvhPrimitives, int *totalNodes, int *orderedPrimitiveOffset, std::vector<Primitive> &orderedPrimitives, int maxPrimsInNode) {
    const auto node = new BVHNode();
    (*totalNodes)++;

    AABB bounds;
    for (const auto &prim: bvhPrimitives) {
        bounds.expand(prim.bounds);
    }

    if (bounds.surfaceArea() == 0 || bvhPrimitives.size() == 1) {
        // CASE: single prim or empty bbox;
        const int firstOffset = *orderedPrimitiveOffset;
        *orderedPrimitiveOffset += bvhPrimitives.size();
        for (size_t i = 0; i < bvhPrimitives.size(); ++i) {
            // const int index                    = ;
            orderedPrimitives[firstOffset + i] = bvhPrimitives[i];
        }
        node->initLeaf(firstOffset, bvhPrimitives.size(), bounds);
        return node;
    } else {
        // Chose split dimensions
        AABB centroidBounds;
        for (const auto &prim: bvhPrimitives) {
            centroidBounds.expand(prim.centroid());
        }
        int dim = centroidBounds.longestAxis();

        if (centroidBounds.pmin[dim] == centroidBounds.pmax[dim]) {
            // CASE: empty bbox
            const int firstOffset = *orderedPrimitiveOffset;
            *orderedPrimitiveOffset += bvhPrimitives.size();
            for (size_t i = 0; i < bvhPrimitives.size(); ++i) {
                // const int index                    = bvhPrimitives[i].index;
                orderedPrimitives[firstOffset + i] = bvhPrimitives[i];
            }
            node->initLeaf(firstOffset, bvhPrimitives.size(), bounds);
            return node;
        }

        int mid = bvhPrimitives.size() / 2;

        if (bvhPrimitives.size() == 2) {
            std::nth_element(
                    bvhPrimitives.begin(),
                    bvhPrimitives.begin() + mid,
                    bvhPrimitives.end(),
                    [dim](const Primitive &a, const Primitive &b) {
                        return a.centroid()[dim] < b.centroid()[dim];
                    });
        } else {
            // Setup buckets
            constexpr int BVH_NUM_BUCKETS = 12;
            BVHBucket buckets[BVH_NUM_BUCKETS];

            for (const auto &prim: bvhPrimitives) {
                int b = BVH_NUM_BUCKETS * centroidBounds.offset(prim.centroid())[dim];
                if (b == BVH_NUM_BUCKETS) b = BVH_NUM_BUCKETS - 1;
                buckets[b].count++;
                buckets[b].bounds.expand(prim.bounds);
            }

            // Setup bucket costs
            constexpr int BVH_NUM_SPLITS = BVH_NUM_BUCKETS - 1;
            float costs[BVH_NUM_SPLITS]  = {};

            // Forward pass
            int countBelow = 0;
            AABB boundsBelow;
            for (int i = 0; i < BVH_NUM_SPLITS; ++i) {
                countBelow += buckets[i].count;
                boundsBelow.expand(buckets[i].bounds);
                costs[i] += countBelow * boundsBelow.surfaceArea();
            }

            // Backwards pass
            int countAbove = 0;
            AABB boundsAbove;
            for (int i = BVH_NUM_BUCKETS - 1; i > 0; --i) {
                countAbove += buckets[i].count;
                boundsAbove.expand(buckets[i].bounds);
                costs[i - 1] += countAbove * boundsAbove.surfaceArea();
            }

            // Find split
            int minBucket = -1;
            float minCost = INF;
            for (int i = 0; i < BVH_NUM_SPLITS; ++i) {
                if (costs[i] < minCost) {
                    minCost   = costs[i];
                    minBucket = i;
                }
            }

            // Calculate split cost
            const float leafCost = bvhPrimitives.size();
            minCost              = 0.5f + minCost / bounds.surfaceArea();
            if (bvhPrimitives.size() > maxPrimsInNode || minCost < leafCost) {
                // Build interior node
                auto midIterator = std::partition(bvhPrimitives.begin(), bvhPrimitives.end(), [=](const Primitive &p) {
                    int b = BVH_NUM_BUCKETS * centroidBounds.offset(p.centroid())[dim];
                    if (b == BVH_NUM_BUCKETS) b = BVH_NUM_BUCKETS - 1;
                    return b <= minBucket;
                });
                mid              = midIterator - bvhPrimitives.begin();
            } else {
                // Build leaf node
                const int firstOffset = *orderedPrimitiveOffset;
                *orderedPrimitiveOffset += bvhPrimitives.size();
                for (size_t i = 0; i < bvhPrimitives.size(); ++i) {
                    // const int index                    = bvhPrimitives[i].index;
                    orderedPrimitives[firstOffset + i] = bvhPrimitives[i];
                }
                node->initLeaf(firstOffset, bvhPrimitives.size(), bounds);
                return node;
            }
        }

        BVHNode *children[2];
        children[0] = buildTree(bvhPrimitives.subspan(0, mid), totalNodes, orderedPrimitiveOffset, orderedPrimitives, maxPrimsInNode);
        children[1] = buildTree(bvhPrimitives.subspan(mid), totalNodes, orderedPrimitiveOffset, orderedPrimitives, maxPrimsInNode);
        node->initBranch(dim, children[0], children[1]);
    }

    return node;
}

int flattenBVH(const BVHNode *node, LinearBVHNode *nodes, int *offset) {
    LinearBVHNode *linearNode = &nodes[*offset];
    linearNode->bbox          = node->bbox;
    const int nodeOffset      = (*offset)++;
    if (node->numPrimitives > 0) {
        linearNode->primitivesOffset = node->firstPrimOffset;
        linearNode->numPrimitives    = node->numPrimitives;
    } else {
        linearNode->axis          = node->splitAxis;
        linearNode->numPrimitives = 0;
        flattenBVH(node->children[0], nodes, offset);
        linearNode->secondChildOffset = flattenBVH(node->children[1], nodes, offset);
    }
    return nodeOffset;
}