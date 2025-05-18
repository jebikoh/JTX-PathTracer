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
    m_scene              = &scene;
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
    linearNode->bbox     = node->bbox;
    const int nodeOffset = (*offset)++;

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

bool BVH2::closestHit(const ray &r, const float t0, float t1, TriangleIntersection &isect) const {
    // PROFILE_SCOPE("BVH2::closestHit");
    int toVisitOffset = 0;
    int currNodeIndex = 0;
    int stack[64];
    bool bHitAnything = false;

    const auto invDir = 1.0f / r.dir;
    int sign[3];
    sign[0] = invDir[0] < 0;
    sign[1] = invDir[1] < 0;
    sign[2] = invDir[2] < 0;

    while (true) {
        const BVH2Node *node = &m_nodes[currNodeIndex];
        if (node->bbox.hit(r, invDir, t0, t1)) {
            if (node->numTriangles > 0) {
                // Leaf node
                for (int i = 0; i < node->numTriangles; ++i) {
                    const auto tri = m_triangles[node->trianglesOffset + i];
                    if (jtx::triangleHit(*m_scene, tri.triangleIndex, r, t0, t1, isect)) {
                        bHitAnything = true;
                        t1          = isect.t;
                    }
                }

                if (toVisitOffset == 0) break;
                currNodeIndex = stack[--toVisitOffset];
            } else {
                // Interior node
                if (sign[node->axis]) {
                    // Process the second child first
                    stack[toVisitOffset++] = currNodeIndex + 1;
                    currNodeIndex          = node->secondChildOffset;
                } else {
                    // Process the first child first
                    stack[toVisitOffset++] = node->secondChildOffset;
                    currNodeIndex          = currNodeIndex + 1;
                }
            }
        } else {
            if (toVisitOffset == 0) break;
            currNodeIndex = stack[--toVisitOffset];
        }
    }

    return bHitAnything;
}

#pragma region BVH4

BVH2BuildNode *buildTreeForBVH4(std::span<Triangle> triangles, int *totalNodes, int *orderedTriangleOffset, std::vector<Triangle> &orderedTriangles) {
    // This is a slightly modified version of buildTree() that adds padding to ensure that the number
    // of triangles in leaf nodes will be a multiple of 4
    const auto node = new BVH2BuildNode();
    (*totalNodes)++;

    // Calculate bounding box for this node
    AABB bbox;
    for (const auto &tri: triangles) {
        bbox.expand(tri.bbox);
    }

    const auto writeLeaf = [&] {
        const int firstOffset = *orderedTriangleOffset;

        const int numTriangles   = triangles.size();
        const int remainder      = numTriangles % 4;
        const int padding        = (remainder == 0) ? 0 : (4 - remainder);
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
            // Add a degenerate triangle -- the default constructor of AABB will create a
            // degenerate bounding box which can't be intersected and won't expand.
            Triangle t{};
            t.triangleIndex                   = -1;
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
    m_nodes = new BVH4Node[totalNodes];

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

bool BVH4::closestHit(const ray &r, float t0, float t1, TriangleIntersection &isect) const {
    int toVisitOffset = 0;
    int currNodeIndex = 0;
    int stack[64];
    bool hitAnything = false;

    AABB4::RayHitInfo rayHitInfo;
    rayHitInfo.invDir  = 1.0f / r.dir;
    rayHitInfo.sign[0] = rayHitInfo.invDir[0] < 0;
    rayHitInfo.sign[1] = rayHitInfo.invDir[1] < 0;
    rayHitInfo.sign[2] = rayHitInfo.invDir[2] < 0;

    while (true) {
        // Process leaf node
        if (currNodeIndex < 0) {
            if (currNodeIndex != JTX_INT_MIN) {
                // Process triangles in batches of 4 (naively)
                const uint32_t numTriangles        = JTX_BVH4_LEAF_NUM_TRIANGLES(currNodeIndex);
                const uint32_t firstTriangleOffset = JTX_BVH4_LEAF_FIRST_TRIANGLE_OFFSET(currNodeIndex);
                for (size_t i = firstTriangleOffset; i < firstTriangleOffset + numTriangles; i += 4) {
                    vfloat4 v0_x, v0_y, v0_z;
                    vfloat4 v1_x, v1_y, v1_z;
                    vfloat4 v2_x, v2_y, v2_z;

                    // Load data into SIMD registers
                    for (int j = 0; j < 4; ++j) {
                        const auto &tri = m_triangles[i + j];
                        if (tri.triangleIndex < 0) continue;

                        vec3 v0 = m_scene->positions[tri.triangleIndex];
                        vec3 v1 = m_scene->positions[tri.triangleIndex + 1];
                        vec3 v2 = m_scene->positions[tri.triangleIndex + 2];

                        v0_x.v[j] = v0.x;
                        v0_y.v[j] = v0.y;
                        v0_z.v[j] = v0.z;

                        v1_x.v[j] = v1.x;
                        v1_y.v[j] = v1.y;
                        v1_z.v[j] = v1.z;

                        v2_x.v[j] = v2.x;
                        v2_y.v[j] = v2.y;
                        v2_z.v[j] = v2.z;
                    }
                    // Intersection test
#ifdef JTX_SIMD_X86_SSE4_2
                    __m128 EPSILON     = _mm_set1_ps(1e-8);
                    __m128 NEG_EPSILON = _mm_set1_ps(-1e-8);
                    __m128 ONE         = _mm_set1_ps(1.0f);
                    __m128 ZERO        = _mm_setzero_ps();

                    __m128 v0x = _mm_load_ps(v0_x.v);
                    __m128 v0y = _mm_load_ps(v0_y.v);
                    __m128 v0z = _mm_load_ps(v0_z.v);
                    __m128 v1x = _mm_load_ps(v1_x.v);
                    __m128 v1y = _mm_load_ps(v1_y.v);
                    __m128 v1z = _mm_load_ps(v1_z.v);
                    __m128 v2x = _mm_load_ps(v2_x.v);
                    __m128 v2y = _mm_load_ps(v2_y.v);
                    __m128 v2z = _mm_load_ps(v2_z.v);

                    __m128 ox  = _mm_set1_ps(r.origin.x);
                    __m128 oy  = _mm_set1_ps(r.origin.y);
                    __m128 oz  = _mm_set1_ps(r.origin.z);
                    __m128 dx  = _mm_set1_ps(r.dir.x);
                    __m128 dy  = _mm_set1_ps(r.dir.y);
                    __m128 dz  = _mm_set1_ps(r.dir.z);
                    __m128 t0v = _mm_set1_ps(t0);
                    __m128 t1v = _mm_set1_ps(t1);

                    // Edge 1
                    __m128 v0v1x = _mm_sub_ps(v1x, v0x);
                    __m128 v0v1y = _mm_sub_ps(v1y, v0y);
                    __m128 v0v1z = _mm_sub_ps(v1z, v0z);

                    // Edge 2
                    __m128 v0v2x = _mm_sub_ps(v2x, v0x);
                    __m128 v0v2y = _mm_sub_ps(v2y, v0y);
                    __m128 v0v2z = _mm_sub_ps(v2z, v0z);

                    // Cross product
                    __m128 crossX = _mm_sub_ps(_mm_mul_ps(dy, v0v2z), _mm_mul_ps(dz, v0v2y));
                    __m128 crossY = _mm_sub_ps(_mm_mul_ps(dz, v0v2x), _mm_mul_ps(dx, v0v2z));
                    __m128 crossZ = _mm_sub_ps(_mm_mul_ps(dx, v0v2y), _mm_mul_ps(dy, v0v2x));

                    // Determinant
                    __m128 det = JTX_SIMD_DOT(crossX, crossY, crossZ, v0v1x, v0v1y, v0v1z);
                    // This should be inverse of the scalar early-out test
                    // If det <= -1e-8 || det >= 1e-8, we want to return 1
                    __m128 detMask = _mm_or_ps(_mm_cmple_ps(det, _mm_set1_ps(-1e-8f)), _mm_cmpge_ps(det, _mm_set1_ps(1e-8f)));
                    __m128 invDet  = _mm_rcp_ps(det);

                    __m128 rx = _mm_sub_ps(ox, v0x);
                    __m128 ry = _mm_sub_ps(oy, v0y);
                    __m128 rz = _mm_sub_ps(oz, v0z);

                    // Barycentric U
                    __m128 b1     = JTX_SIMD_DOT(rx, ry, rz, crossX, crossY, crossZ);
                    __m128 b1Mask = _mm_and_ps(_mm_cmpge_ps(b1, _mm_setzero_ps()), _mm_cmple_ps(b1, _mm_set1_ps(1.0f)));

                    __m128 qx = _mm_sub_ps(_mm_mul_ps(ry, v0v1z), _mm_mul_ps(rz, v0v1y));
                    __m128 qy = _mm_sub_ps(_mm_mul_ps(rz, v0v1x), _mm_mul_ps(rx, v0v1z));
                    __m128 qz = _mm_sub_ps(_mm_mul_ps(rx, v0v1y), _mm_mul_ps(rz, v0v1x));

                    // Barycentric V
                    __m128 b2     = _mm_mul_ps(JTX_SIMD_DOT(dx, dy, dz, qx, qy, qz), invDet);
                    __m128 b2Mask = _mm_and_ps(_mm_cmpge_ps(b2, _mm_setzero_ps()), _mm_cmple_ps(b2, _mm_set1_ps(1.0f)));

                    // Calculate root
                    __m128 root     = _mm_mul_ps(JTX_SIMD_DOT(v0v2x, v0v2y, v0v2z, qx, qy, qz), invDet);
                    __m128 rootMask = _mm_and_ps(_mm_cmplt_ps(t0v, root), _mm_cmplt_ps(root, t1v));

                    __m128 mask = _mm_and_ps(_mm_and_ps(detMask, b1Mask), _mm_and_ps(b2Mask, rootMask));

                    // Get results for t, u, v
                    __m128 t4 = _mm_set1_ps(-1.0f);
                    __m128 u4 = _mm_set1_ps(-1.0f);
                    __m128 v4 = _mm_set1_ps(-1.0f);

                    t4 = _mm_blendv_ps(t4, root, mask);
                    u4 = _mm_blendv_ps(u4, b1, mask);
                    v4 = _mm_blendv_ps(v4, b2, mask);

                    float t[4];
                    _mm_store_ps(t, t4);

                    // Find the closest hit (if any)
                    int closestHitIndex = -1;
                    float closestHit    = JTX_INFINITY_F;
                    for (int k = 0; k < 4; ++k) {
                        if (t[k] > 0.0f && t[k] < closestHit) {
                            closestHit      = t[k];
                            closestHitIndex = k;
                        }
                    }
#endif
                }
            }

            if (toVisitOffset == 0) break;
            currNodeIndex = stack[--toVisitOffset];
        }

        const BVH4Node *node = &m_nodes[currNodeIndex];

        const auto hitResult = node->bbox.hit(r, rayHitInfo, t0, t1);
        if (hitResult.val > 0) {
            int order[4] = {0, 1, 2, 3};
            // Now for the tough part, sorting the boxes
            // We want to sort the boxes by the split axes
            if (rayHitInfo.sign[node->axis[0]]) {
                // (0, 1, 2, 3) -> (2, 3, 0, 1)
                std::swap(order[0], order[2]);
                std::swap(order[1], order[3]);

                if (node->axis[1] != -1) {
                    if (rayHitInfo.sign[node->axis[1]]) {
                        // (2, 3, 0, 1) -> (3, 2, _, _)
                        std::swap(order[2], order[3]);
                    }
                }

                if (node->axis[2] != -1) {
                    if (rayHitInfo.sign[node->axis[2]]) {
                        // (_, _, 0, 1) -> (_, _, 1, 0)
                        std::swap(order[0], order[1]);
                    }
                }
            } else {
                if (node->axis[1] != -1) {
                    // (0, 1, _, _) -> (1, 0, _, _)
                    if (rayHitInfo.sign[node->axis[1]]) {
                        std::swap(order[0], order[1]);
                    }
                }

                if (node->axis[2] != -1) {
                    // (_, _, 2, 3) -> (_, _, 3, 2)
                    if (rayHitInfo.sign[node->axis[2]]) {
                        std::swap(order[2], order[3]);
                    }
                }
            }

            // Push the children to the stack
            for (int i = 0; i < 4; ++i) {
                const int child = order[i];
                if (hitResult.bHit[order[child]]) {
                    stack[toVisitOffset++] = node->children[child];
                }
            }
            currNodeIndex = stack[--toVisitOffset];
        } else {
            if (toVisitOffset == 0) break;
            currNodeIndex = stack[--toVisitOffset];
        }
    }

    return hitAnything;
}

#pragma endregion

}// namespace jtx
