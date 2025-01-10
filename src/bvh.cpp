#include "bvh.hpp"

BVHTree::BVHTree(const Scene &scene, const int maxPrimsInNode)
    : maxPrimsInNode_(maxPrimsInNode),
      scene_(scene) {
    // This is our global list of primitives
    primitives_.resize(scene.numPrimitives());
    // std::vector<Primitive> primitives(scene.numPrimitives());

    // BVH Primitives is our working span of primitives
    // This will start out as all of them
    std::vector<Primitive> bvhPrimitives(primitives_.size());
    for (size_t i = 0; i < scene.spheres.size(); ++i) {
        primitives_[i]   = Primitive{Primitive::SPHERE, i, scene.spheres[i].bounds()};
        bvhPrimitives[i] = Primitive{Primitive::SPHERE, i, scene.spheres[i].bounds()};
    }
    const size_t j = scene.spheres.size();
    for (size_t i = 0; i < scene.triangles.size(); ++i) {
        primitives_[j + i]   = Primitive{Primitive::TRIANGLE, i, scene.meshes[scene.triangles[i].meshIndex].tBounds(scene.triangles[i].index)};
        bvhPrimitives[j + i] = Primitive{Primitive::TRIANGLE, i, scene.meshes[scene.triangles[i].meshIndex].tBounds(scene.triangles[i].index)};
    }
    // Add rest of types when we get them
    // We will order as we build
    std::vector<Primitive> orderedPrimitives(primitives_.size());

    int totalNodes             = 1;
    int orderedPrimitiveOffset = 0;

    const BVHNode *root = buildTree(bvhPrimitives, &totalNodes, &orderedPrimitiveOffset, orderedPrimitives);
    primitives_.swap(orderedPrimitives);

    bvhPrimitives.resize(0);
    bvhPrimitives.shrink_to_fit();

    nodes_     = new LinearBVHNode[totalNodes];
    int offset = 0;
    flattenBVH(root, &offset);

    // Clean-up the tree
    root->destroy();
    delete root;
}

struct BVHBucket {
    int count = 0;
    AABB bounds;
};

bool BVHTree::hit(const Ray &r, Interval t, HitRecord &record) const {
    const auto invDir     = 1 / r.dir;
    const int dirIsNeg[3] = {static_cast<int>(invDir.x < 0), static_cast<int>(invDir.y < 0), static_cast<int>(invDir.z < 0)};

    int toVisitOffset    = 0;
    int currentNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    while (true) {
        const LinearBVHNode *node = &nodes_[currentNodeIndex];
        // 1. Check the ray intersects the current node
        //    If it doesn't, pop the stack and continue
        if (node->bbox.hit(r.origin, r.dir, t)) {
            // 2. If we are at a leaf node, loop through all primitives
            //    Otherwise, push the children onto the stack
            if (node->numPrimitives > 0) {
                // Leaf node
                for (int i = 0; i < node->numPrimitives; ++i) {
                    if (scene_.hit(primitives_[node->primitivesOffset + i], r, t, record)) {
                        hitAnything = true;
                        t.max       = record.t;
                    }
                }
                if (toVisitOffset == 0) break;
                currentNodeIndex = stack[--toVisitOffset];
            } else {
                // Interior node
                if (dirIsNeg[node->axis]) {
                    stack[toVisitOffset++] = currentNodeIndex + 1;
                    currentNodeIndex       = node->secondChildOffset;
                } else {
                    stack[toVisitOffset++] = node->secondChildOffset;
                    currentNodeIndex       = currentNodeIndex + 1;
                }
            }
        } else {
            if (toVisitOffset == 0) break;
            currentNodeIndex = stack[--toVisitOffset];
        }
    }

    return hitAnything;
}

BVHNode *BVHTree::buildTree(std::span<Primitive> bvhPrimitives, int *totalNodes, int *orderedPrimitiveOffset, std::vector<Primitive> &orderedPrimitives) {
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
            const int index                    = bvhPrimitives[i].index;
            orderedPrimitives[firstOffset + i] = primitives_[index];
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
                const int index                    = bvhPrimitives[i].index;
                orderedPrimitives[firstOffset + i] = primitives_[index];
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
            if (bvhPrimitives.size() > maxPrimsInNode_ || minCost < leafCost) {
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
                    const int index                    = bvhPrimitives[i].index;
                    orderedPrimitives[firstOffset + i] = primitives_[index];
                }
                node->initLeaf(firstOffset, bvhPrimitives.size(), bounds);
                return node;
            }
        }

        BVHNode *children[2];
        children[0] = buildTree(bvhPrimitives.subspan(0, mid), totalNodes, orderedPrimitiveOffset, orderedPrimitives);
        children[1] = buildTree(bvhPrimitives.subspan(mid), totalNodes, orderedPrimitiveOffset, orderedPrimitives);
        node->initBranch(dim, children[0], children[1]);
    }

    return node;
}

int BVHTree::flattenBVH(const BVHNode *node, int *offset) {
    LinearBVHNode *linearNode = &nodes_[*offset];
    linearNode->bbox          = node->bbox;
    const int nodeOffset      = (*offset)++;
    if (node->numPrimitives > 0) {
        linearNode->primitivesOffset = node->firstPrimOffset;
        linearNode->numPrimitives    = node->numPrimitives;
    } else {
        linearNode->axis          = node->splitAxis;
        linearNode->numPrimitives = 0;
        flattenBVH(node->children[0], offset);
        linearNode->secondChildOffset = flattenBVH(node->children[1], offset);
    }
    return nodeOffset;
}