#include "bvh.hpp"

#include "scene/scene.hpp"

namespace jtx {

struct BVHBucket {
    int count = 0;
    AABB bbox;
};

void BVH2::build(jtx::Scene &scene, int maxTrianglesInNode) {
    LOG_INFO(GENERAL, "Building BVH2 for scene: {}", scene.name);
    PROFILE_SCOPE("bvh::build");
    m_scene = &scene;
    m_maxTrianglesInNode = maxTrianglesInNode;
    m_triangles          = scene.getTriangles();

    std::vector<Triangle> orderedTriangles(m_triangles.size());

    int totalNodes            = 1;
    int orderedTriangleOffset = 0;

    LOG_DEBUG(GENERAL, "Building recursive BVH2 structure");
    const BVH2Node *root = buildTree(m_triangles, &totalNodes, &orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    m_triangles.swap(orderedTriangles);

    m_nodes    = new LBVH2Node[totalNodes];
    int offset = 0;
    LOG_INFO(GENERAL, "Flattening BVH2");
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
    LOG_INFO(GENERAL, "Destroyed BVH2");
}

BVH2Node *buildTree(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles, int maxTrianglesInNode) {
    const auto node = new BVH2Node();
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

    BVH2Node *children[2];
    children[0] = buildTree(triangles.subspan(0, mid), totalNodes, orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    children[1] = buildTree(triangles.subspan(mid), totalNodes, orderedTriangleOffset, orderedTriangles, maxTrianglesInNode);
    node->initBranch(dim, children[0], children[1]);

    return node;
}

int flattenBVH2(const BVH2Node *node, LBVH2Node *nodes, int *offset) {
    LBVH2Node *linearNode = &nodes[*offset];
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
    const vec3 invDir     = 1 / r.dir;
    const int dirIsNeg[3] = {
        static_cast<int>(invDir.x < 0),
        static_cast<int>(invDir.y < 0),
        static_cast<int>(invDir.z < 0)};

    int toVisitOffset = 0;
    int currNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    while (true) {
        const LBVH2Node *node = &m_nodes[currNodeIndex];
        if (node->bbox.hit(r.origin, r.dir, t0, t1)) {
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
                if (dirIsNeg[node->axis]) {
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

}// namespace jtx