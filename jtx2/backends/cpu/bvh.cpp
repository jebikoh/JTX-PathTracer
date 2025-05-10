#include "bvh.hpp"

#include "scene/scene.hpp"

namespace jtx {

struct BVHBucket {
    int count = 0;
    AABB bbox;
};

void BVH2::build(const jtx::Scene &scene, int maxTrianglesInNode) {
    LOG_INFO(GENERAL, "Building BVH2 for scene: {}", scene.name);
    PROFILE_SCOPE("bvh::build");
    m_scene = &scene;
    m_maxTrianglesInNode = maxTrianglesInNode;
    m_triangles          = scene.getTriangles();

    std::vector<Triangle> orderedTriangles(m_triangles.size());

    int totalNodes            = 1;
    int orderedTriangleOffset = 0;

    LOG_DEBUG(GENERAL, "Building recursive BVH2 structure");
    const BVH2BuildNode *root = buildTree(m_triangles, &totalNodes, &orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    m_triangles.swap(orderedTriangles);

    m_nodes    = new BVH2Node[totalNodes];
    int offset = 0;
    LOG_DEBUG(GENERAL, "Flattening BVH2");
    flattenBVH2(root, m_nodes, &offset);
    LOG_INFO(GENERAL, "BVH2 constructed");

    root->destroy();
    delete root;
}

void BVH2::destroy() {
    LOG_INFO(GENERAL, "Destroying BVH2");
    if (m_nodes != nullptr) {
        delete[] m_nodes;
        m_nodes = nullptr;
    }

    m_triangles.clear();
    m_triangles.shrink_to_fit();
    LOG_INFO(GENERAL, "Destroyed BVH2");
}

BVH2BuildNode *buildTree(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles, int maxTrianglesInNode) {
    const auto node = new BVH2BuildNode();
    (*totalNodes)++;

    // Calculate bounding box for this node
    AABB bbox;
    for (const auto &tri: triangles) {
        bbox.expand(tri.bbox);
    }

    // Initialize leaf
    if (bbox.surfaceArea() == 0 || triangles.size() == 1) {
        const int firstOffset = *orderedTriangleOffset;
        *orderedTriangleOffset += triangles.size();
        for (size_t i = 0; i < triangles.size(); i++) {
            orderedTriangles[firstOffset + i] = triangles[i];
        }
        node->initLeaf(firstOffset, triangles.size(), bbox);
        return node;
    }

    AABB centroidBbox;
    for (const auto &tri: triangles) {
        centroidBbox.expand(tri.centroid());
    }
    const int dim = centroidBbox.longestAxis();

    // Initialize leaf
    if (centroidBbox.pmin[dim] == centroidBbox.pmax[dim]) {
        const int firstOffset = *orderedTriangleOffset;
        *orderedTriangleOffset += triangles.size();
        for (size_t i = 0; i < triangles.size(); i++) {
            orderedTriangles[firstOffset + i] = triangles[i];
        }
        node->initLeaf(firstOffset, triangles.size(), bbox);
        return node;
    }

    int mid = triangles.size() / 2;
    if (triangles.size() == 2) {
        std::nth_element(triangles.begin(), triangles.begin() + mid, triangles.end(), [dim](const Triangle &a, const Triangle &b) {
            return a.centroid()[dim] < b.centroid()[dim];
        });
    } else {
        constexpr int JTX_BVH2_NUM_BUCKETS = 12;
        BVHBucket buckets[JTX_BVH2_NUM_BUCKETS];

        for (const auto &tri: triangles) {
            int bucketOffset = JTX_BVH2_NUM_BUCKETS * centroidBbox.offset(tri.centroid())[dim];
            if (bucketOffset == JTX_BVH2_NUM_BUCKETS) --bucketOffset;
            buckets[bucketOffset].count++;
            buckets[bucketOffset].bbox.expand(tri.bbox);
        }

        constexpr int JTX_BVH2_NUM_SPLITS = JTX_BVH2_NUM_BUCKETS - 1;
        float costs[JTX_BVH2_NUM_SPLITS]  = {};

        int count = 0;
        AABB below;
        for (int i = 0; i < JTX_BVH2_NUM_SPLITS; ++i) {
            count += buckets[i].count;
            below.expand(buckets[i].bbox);
            const float surfaceArea = below.surfaceArea();
            costs[i] += count * surfaceArea;
        }

        count = 0;
        AABB above;
        for (int i = JTX_BVH2_NUM_BUCKETS - 1; i > 0; --i) {
            count += buckets[i].count;
            above.expand(buckets[i].bbox);
            costs[i - 1] += count * above.surfaceArea();
        }

        int minBucket = -1;
        float minCost = JTX_INFINITY_F;
        for (int i = 0; i < JTX_BVH2_NUM_SPLITS; ++i) {
            if (costs[i] < minCost) {
                minBucket = i;
                minCost   = costs[i];
            }
        }

        const float leafCost = triangles.size();
        minCost              = 0.5f + minCost / bbox.surfaceArea();
        if (triangles.size() > maxTrianglesInNode || minCost < leafCost) {
            // Interior node
            auto it = std::partition(triangles.begin(), triangles.end(), [=](const Triangle &p) {
                int b = JTX_BVH2_NUM_BUCKETS * centroidBbox.offset(p.centroid())[dim];
                if (b == JTX_BVH2_NUM_BUCKETS) --b;
                return b <= minBucket;
            });
            mid     = it - triangles.begin();
        } else {
            const int firstOffset = *orderedTriangleOffset;
            *orderedTriangleOffset += triangles.size();
            for (size_t i = 0; i < triangles.size(); i++) {
                orderedTriangles[firstOffset + i] = triangles[i];
            }
            node->initLeaf(firstOffset, triangles.size(), bbox);
            return node;
        }
    }

    BVH2BuildNode *children[2];
    children[0] = buildTree(triangles.subspan(0, mid), totalNodes, orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    children[1] = buildTree(triangles.subspan(mid), totalNodes, orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    node->initBranch(dim, children[0], children[1]);

    return node;
}

int flattenBVH2(const BVH2BuildNode *node, BVH2Node *nodes, int *offset) {
    BVH2Node *linearNode = &nodes[*offset];
    linearNode->bbox      = node->bbox;
    const int nodeOffset  = (*offset)++;

    if (node->numTriangles > 0) {
        linearNode->trianglesOffset = node->firstTriangleOffset;
        linearNode->numTriangles    = node->numTriangles;
    } else {
        linearNode->axis         = node->splitAxis;
        linearNode->numTriangles = 0;
        flattenBVH2(node->children[0], nodes, offset);
        linearNode->secondChildOffset = flattenBVH2(node->children[1], nodes, offset);
    }
    return nodeOffset;
}

bool BVH2::closestHit(const ray &r, const float t0, float t1, SurfaceIntersection &isect) const {
    // PROFILE_SCOPE("BVH2::closestHit");
    int toVisitOffset = 0;
    int currNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    while (true) {
        const BVH2Node *node = &m_nodes[currNodeIndex];
        if (node->bbox.hit(r, t0, t1)) {
            if (node->numTriangles > 0) {
                // Leaf node
                for (int i = 0; i < node->numTriangles; ++i) {
                    const auto tri = m_triangles[node->trianglesOffset + i];
                    if (jtx::tClosestHit(*m_scene, tri.triangleIndex, r, t0, t1, isect)) {
                        hitAnything = true;
                        t1 = isect.t;
                    }
                }

                if (toVisitOffset == 0) break;
                currNodeIndex = stack[--toVisitOffset];
            } else {
                // Interior node
                if (r.sign[node->axis]) {
                    stack[toVisitOffset++] = currNodeIndex + 1;
                    currNodeIndex = node->secondChildOffset;
                } else {
                    stack[toVisitOffset++] = node->secondChildOffset;
                    currNodeIndex = currNodeIndex + 1;
                }
            }
        } else {
            if (toVisitOffset == 0) break;
            currNodeIndex = stack[--toVisitOffset];
        }
    }

    return hitAnything;
}

BVH2BuildNode *buildTreeForBVH4(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles) {
    // This is a slightly modified version of buildTree() that adds padding to ensure that the number
    // of trangles in leaf nodes will be a multiple of 4
    const auto node = new BVH2BuildNode();
    (*totalNodes)++;

    // Calculate bounding box for this node
    AABB bbox;
    for (const auto &tri: triangles) {
        bbox.expand(tri.bbox);
    }

    const auto writeLeaf = [&] {
        const int firstOffset = *orderedTriangleOffset;

        const int numTriangles = triangles.size();
        const int remainder = numTriangles % 4;
        const int padding = (remainder == 0) ? 0 : (4 - remainder);
        const int totalTriangles = numTriangles + padding;

        // Check if we are about run out of space
        if (firstOffset + totalTriangles > orderedTriangles.size()) {
            orderedTriangles.resize(orderedTriangles.size() * 1.5);
        }

        *orderedTriangleOffset += totalTriangles;

        for (size_t i = 0; i < numTriangles; i++) {
            orderedTriangles[firstOffset + i] = triangles[i];
        }
        for (size_t i = numTriangles; i < totalTriangles; i++) {
            // Add a degenerate triangle
            Triangle t{};
            t.triangleIndex = -1;
            orderedTriangles[firstOffset + i] = t;
        }

        node->initLeaf(firstOffset, totalTriangles, bbox);
        return node;
    };

    // Initialize leaf
    if (bbox.surfaceArea() == 0 || triangles.size() == 1) {
        return writeLeaf();
    }

    AABB centroidBbox;
    for (const auto &tri: triangles) {
        centroidBbox.expand(tri.centroid());
    }
    const int dim = centroidBbox.longestAxis();

    // Initialize leaf
    if (centroidBbox.pmin[dim] == centroidBbox.pmax[dim]) {
        return writeLeaf();
    }

    int mid = triangles.size() / 2;
    if (triangles.size() == 2) {
        std::nth_element(triangles.begin(), triangles.begin() + mid, triangles.end(), [dim](const Triangle &a, const Triangle &b) {
            return a.centroid()[dim] < b.centroid()[dim];
        });
    } else {
        constexpr int JTX_BVH2_NUM_BUCKETS = 12;
        BVHBucket buckets[JTX_BVH2_NUM_BUCKETS];

        for (const auto &tri: triangles) {
            int bucketOffset = JTX_BVH2_NUM_BUCKETS * centroidBbox.offset(tri.centroid())[dim];
            if (bucketOffset == JTX_BVH2_NUM_BUCKETS) --bucketOffset;
            buckets[bucketOffset].count++;
            buckets[bucketOffset].bbox.expand(tri.bbox);
        }

        constexpr int JTX_BVH2_NUM_SPLITS = JTX_BVH2_NUM_BUCKETS - 1;
        float costs[JTX_BVH2_NUM_SPLITS]  = {};

        int count = 0;
        AABB below;
        for (int i = 0; i < JTX_BVH2_NUM_SPLITS; ++i) {
            count += buckets[i].count;
            below.expand(buckets[i].bbox);
            const float surfaceArea = below.surfaceArea();
            costs[i] += count * surfaceArea;
        }

        count = 0;
        AABB above;
        for (int i = JTX_BVH2_NUM_BUCKETS - 1; i > 0; --i) {
            count += buckets[i].count;
            above.expand(buckets[i].bbox);
            costs[i - 1] += count * above.surfaceArea();
        }

        int minBucket = -1;
        float minCost = JTX_INFINITY_F;
        for (int i = 0; i < JTX_BVH2_NUM_SPLITS; ++i) {
            if (costs[i] < minCost) {
                minBucket = i;
                minCost   = costs[i];
            }
        }

        const float leafCost = triangles.size();
        minCost              = 0.5f + minCost / bbox.surfaceArea();
        if (triangles.size() > 64 || minCost < leafCost) {
            // Interior node
            auto it = std::partition(triangles.begin(), triangles.end(), [=](const Triangle &p) {
                int b = JTX_BVH2_NUM_BUCKETS * centroidBbox.offset(p.centroid())[dim];
                if (b == JTX_BVH2_NUM_BUCKETS) --b;
                return b <= minBucket;
            });
            mid     = it - triangles.begin();
        } else {
            // Write leaf
            return writeLeaf();
        }
    }

    BVH2BuildNode *children[2];
    children[0] = buildTreeForBVH4(triangles.subspan(0, mid), totalNodes, orderedTriangleOffset, orderedTriangles);
    children[1] = buildTreeForBVH4(triangles.subspan(mid), totalNodes, orderedTriangleOffset, orderedTriangles);
    node->initBranch(dim, children[0], children[1]);

    return node;
}

inline int32_t encodeBVH4Leaf(const BVH2BuildNode *node) {
    // Assume the BVH2 triangles is a multiple of 4 and is <=64
    ASSERT(node->numTriangles <= 64 && node->numTriangles % 4 == 0);

    const uint32_t numTriangles = static_cast<uint32_t>(node->numTriangles / 4 - 1) & 0xF;
    return 0x80000000 | (numTriangles << JTX_BVH4_FIRST_INDEX_BIT_WIDTH) | (static_cast<uint32_t>(node->firstTriangleOffset) & JTX_BVH4_FIRST_INDEX_MASK);
}

int flattenBVH2toLBVH4(const BVH2BuildNode *node, BVH4Node *nodes, int *offset) {
    // If the node is a leaf, we need to avoid incrementing the offset
    if (node->isLeaf()) return encodeBVH4Leaf(node);

    const int nodeOffset = (*offset)++;
    BVH4Node *ln         = &nodes[nodeOffset];

    // We want to collapse *2* levels of the original BVH tree
    const BVH2BuildNode *left  = node->children[0];
    const BVH2BuildNode *right = node->children[1];

    const bool bLeftIsLeaf  = left->isLeaf();
    const bool bRightIsLeaf = right->isLeaf();

    const BVH2BuildNode *n[4];
    n[0] = bLeftIsLeaf ? left : left->children[0];
    n[1] = bLeftIsLeaf ? nullptr : left->children[1];
    n[2] = bRightIsLeaf ? right : right->children[0];
    n[3] = bRightIsLeaf ? nullptr : right->children[1];

    // Now iterate and populate the bbox. For empty nodes, we want to either mask
    // the lanes or use a bbox that will never return true (this is possible)
    for (size_t i = 0; i < 4; ++i) {
        if (n[i] != nullptr) {
            const auto bbox       = n[i]->bbox;
            ln->bbox.pmax[0].v[i] = bbox.pmax.x;
            ln->bbox.pmax[1].v[i] = bbox.pmax.y;
            ln->bbox.pmax[2].v[i] = bbox.pmax.z;
            ln->bbox.pmin[0].v[i] = bbox.pmin.x;
            ln->bbox.pmin[1].v[i] = bbox.pmin.y;
            ln->bbox.pmin[2].v[i] = bbox.pmin.z;

            ln->children[i] = flattenBVH2toLBVH4(n[i], nodes, offset);
        } else {
            // To denote this as an empty leaf, we use INT_MIN
            ln->children[i]       = JTX_INT_MIN;
            ln->bbox.pmax[0].v[i] = JTX_NEG_INFINITY_F;
            ln->bbox.pmax[1].v[i] = JTX_NEG_INFINITY_F;
            ln->bbox.pmax[2].v[i] = JTX_NEG_INFINITY_F;
            ln->bbox.pmin[0].v[i] = JTX_INFINITY_F;
            ln->bbox.pmin[1].v[i] = JTX_INFINITY_F;
            ln->bbox.pmin[2].v[i] = JTX_INFINITY_F;
        }
    }

    // Keep track of the split axes for efficient traversal
    ln->axis[0] = node->splitAxis;
    ln->axis[1] = bLeftIsLeaf ? -1 : left->splitAxis;
    ln->axis[2] = bRightIsLeaf ? -1 : right->splitAxis;

    return nodeOffset;
}

void BVH4::build(const jtx::Scene &scene) {
    LOG_INFO(GENERAL, "Building BVH4 for scene: {}", scene.name);
    PROFILE_SCOPE("bvh::build");
    m_scene     = &scene;
    m_triangles = scene.getTriangles();

    // TODO: optimize memory for BVH4
    std::vector<Triangle> orderedTriangles(m_triangles.size() * 1.75);

    int totalNodes            = 1;
    int orderedTriangleOffset = 0;

    LOG_DEBUG(GENERAL, "Building recursive BVH2 structure");
    const BVH2BuildNode *root = buildTreeForBVH4(m_triangles, &totalNodes, &orderedTriangleOffset, orderedTriangles);
    m_triangles.swap(orderedTriangles);


    // TODO: optimize memory for BVH4
    m_nodes    = new BVH4Node[totalNodes];

    int offset = 0;
    LOG_DEBUG(GENERAL, "Flattening BVH2 to LBVH4");
    flattenBVH2toLBVH4(root, m_nodes, &offset);
    LOG_INFO(GENERAL, "BVH4 constructed");

    root->destroy();
    delete root;
}

void BVH4::destroy() {
    LOG_INFO(GENERAL, "Destroying BVH4");
    if (m_nodes != nullptr) {
        delete[] m_nodes;
        m_nodes = nullptr;
    }

    m_triangles.clear();
    m_triangles.shrink_to_fit();
    LOG_INFO(GENERAL, "Destroyed BVH4");
}

bool BVH4::closestHit(const ray &r, float t0, float t1, SurfaceIntersection &isect) const {
    int toVisitOffset = 0;
    int currNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    while (true) {
        const BVH4Node *node = &m_nodes[currNodeIndex];

        const auto hitResult = node->bbox.hit(r, t0, t1);
        if (hitResult.val > 0) {
            // Now for the tough part, sorting the boxes
            // We want to sort the boxes by the split axes
            int childOrder[4];


        } else {
            if (toVisitOffset == 0) break;
            currNodeIndex = stack[--toVisitOffset];
        }
    }

    return hitAnything;
}

}// namespace jtx
