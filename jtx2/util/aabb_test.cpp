#include "aabb.hpp"
#include "rng.hpp"
#include <gtest/gtest.h>

namespace jtx {

constexpr uint32_t JTX_AABB4_TEST_NUM_SAMPLES = 10000;

// Use CMake options to test various implementations
TEST(AABB4, HitResultMatchesAABB) {
    RNG rng;
    // Generate 4 points for xyz in [-1, 1]
    vec3 pmin[4];
    rng.range(-1, 1, pmin[0]);
    rng.range(-1, 1, pmin[1]);
    rng.range(-1, 1, pmin[2]);
    rng.range(-1, 1, pmin[3]);

    vec3 pmax[4];
    rng.range(-1, 1, pmax[0]);
    rng.range(-1, 1, pmax[1]);
    rng.range(-1, 1, pmax[2]);
    rng.range(-1, 1, pmax[3]);

    // Create generic single AABBs
    AABB ref[4];
    ref[0] = {pmin[0], pmax[0]};
    ref[1] = {pmin[1], pmax[1]};
    ref[2] = {pmin[2], pmax[2]};
    ref[3] = {pmin[3], pmax[3]};

    // AABB4
    AABB4 bbox4;

    // Swizzle data for AABB4
    bbox4.pmin[0].v[0] = ref[0].pmin.x;
    bbox4.pmin[0].v[1] = ref[1].pmin.x;
    bbox4.pmin[0].v[2] = ref[2].pmin.x;
    bbox4.pmin[0].v[3] = ref[3].pmin.x;

    bbox4.pmin[1].v[0] = ref[0].pmin.y;
    bbox4.pmin[1].v[1] = ref[1].pmin.y;
    bbox4.pmin[1].v[2] = ref[2].pmin.y;
    bbox4.pmin[1].v[3] = ref[3].pmin.y;

    bbox4.pmin[2].v[0] = ref[0].pmin.z;
    bbox4.pmin[2].v[1] = ref[1].pmin.z;
    bbox4.pmin[2].v[2] = ref[2].pmin.z;
    bbox4.pmin[2].v[3] = ref[3].pmin.z;

    bbox4.pmax[0].v[0] = ref[0].pmax.x;
    bbox4.pmax[0].v[1] = ref[1].pmax.x;
    bbox4.pmax[0].v[2] = ref[2].pmax.x;
    bbox4.pmax[0].v[3] = ref[3].pmax.x;

    bbox4.pmax[1].v[0] = ref[0].pmax.y;
    bbox4.pmax[1].v[1] = ref[1].pmax.y;
    bbox4.pmax[1].v[2] = ref[2].pmax.y;
    bbox4.pmax[1].v[3] = ref[3].pmax.y;

    bbox4.pmax[2].v[0] = ref[0].pmax.z;
    bbox4.pmax[2].v[1] = ref[1].pmax.z;
    bbox4.pmax[2].v[2] = ref[2].pmax.z;
    bbox4.pmax[2].v[3] = ref[3].pmax.z;

    ray rays[JTX_AABB4_TEST_NUM_SAMPLES];
    for (int i = 0; i < JTX_AABB4_TEST_NUM_SAMPLES; ++i) {
        rng.range(-1.5, 1.5, rays[i].origin);
        rays[i].dir = rng.unitVector();
        rays[i].time = 0;
    }

    uint32_t numMatches = 0;
    uint32_t numHits = 0;
    uint32_t numMisses = 0;

    for (const auto &r : rays) {
        AABB4::RayHitInfo rayHitInfo;
        rayHitInfo.invDir = 1.0f / r.dir;
        rayHitInfo.sign[0] = rayHitInfo.invDir[0] < 0;
        rayHitInfo.sign[1] = rayHitInfo.invDir[1] < 0;
        rayHitInfo.sign[2] = rayHitInfo.invDir[2] < 0;

        bool refHits[4];
        for (int i = 0; i < 4; ++i) {
            refHits[i] = ref[i].hit(r, rayHitInfo.invDir,0.0f, JTX_INFINITY_F);
        }

        const auto res = bbox4.hit(r, rayHitInfo,0.0f, JTX_INFINITY_F);

        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(refHits[i], res.bHit[i]);
            if (refHits[i] == res.bHit[i]) {
                numMatches++;
                if (refHits[i]) numHits++;
                else numMisses++;
            } else {
                LOG_DEBUG(TEST, "Mismatch for ray: ({},{},{}) to ({},{},{})", r.origin.x, r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z);
            }
        }
    }

    EXPECT_GT(numMisses, 0u) << "Every ray resulted in a hit -- statistically unlikely";

    LOG_DEBUG(GENERAL, "Num matches: {}", numMatches);
    LOG_DEBUG(GENERAL, "Pct Match: {}%", static_cast<float>(numMatches) / (JTX_AABB4_TEST_NUM_SAMPLES * 4) * 100);
    LOG_DEBUG(GENERAL, "Matching Hits: {}", numHits);
    LOG_DEBUG(GENERAL, "Matching Misses: {}", numMisses);
}

}

