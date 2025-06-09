#include <benchmark/benchmark.h>
#include <util/aabb.hpp>
#include <util/sampling.hpp>

using namespace jtx;

static constexpr size_t NUM_RAYS  = 1 << 18;
static constexpr size_t NUM_BOXES = 1 << 14;// This MUST be a multiple of 4
static constexpr uint32_t SEED    = 1234567;

class AABBFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State &state) override {
        rays    = new ray[NUM_RAYS];
        rayInfo = new AABB4::RayHitInfo[NUM_RAYS];
        boxes   = new AABB[NUM_BOXES];
        boxes4  = new AABB4[NUM_BOXES / 4];
        rng     = Sampler(SEED);

        // Setup rays
        for (int i = 0; i < NUM_RAYS; ++i) {
            rays[i].origin = rng.Uniform<vec3>(-2.0f, 2.0f);
            rays[i].dir  = unitVector(rng.Uniform<float>(), rng.Uniform<float>());
            rays[i].time = 0.0f;

            rayInfo[i].invDir  = 1.0f / rays[i].dir;
            rayInfo[i].sign[0] = rayInfo[i].invDir[0] < 0;
            rayInfo[i].sign[1] = rayInfo[i].invDir[1] < 0;
            rayInfo[i].sign[2] = rayInfo[i].invDir[2] < 0;
        }

        // Setup AABBs
        for (int i = 0; i < NUM_BOXES; ++i) {
            auto pmin = rng.Uniform<vec3>(-1.0f, 1.0f);
            auto pmax = rng.Uniform<vec3>(-1.0f, 1.0f);
            boxes[i] = AABB(pmin, pmax);
        }

        // Setup AABB4
        for (int i = 0; i < NUM_BOXES / 4; i += 4) {
            const size_t j = i * 4;

            AABB4 &bbox4       = boxes4[i];
            bbox4.pmin[0].v[0] = boxes[j + 0].pmin.x;
            bbox4.pmin[0].v[1] = boxes[j + 1].pmin.x;
            bbox4.pmin[0].v[2] = boxes[j + 2].pmin.x;
            bbox4.pmin[0].v[3] = boxes[j + 3].pmin.x;

            bbox4.pmin[1].v[0] = boxes[j + 0].pmin.y;
            bbox4.pmin[1].v[1] = boxes[j + 1].pmin.y;
            bbox4.pmin[1].v[2] = boxes[j + 2].pmin.y;
            bbox4.pmin[1].v[3] = boxes[j + 3].pmin.y;

            bbox4.pmin[2].v[0] = boxes[j + 0].pmin.z;
            bbox4.pmin[2].v[1] = boxes[j + 1].pmin.z;
            bbox4.pmin[2].v[2] = boxes[j + 2].pmin.z;
            bbox4.pmin[2].v[3] = boxes[j + 3].pmin.z;

            bbox4.pmax[0].v[0] = boxes[j + 0].pmax.x;
            bbox4.pmax[0].v[1] = boxes[j + 1].pmax.x;
            bbox4.pmax[0].v[2] = boxes[j + 2].pmax.x;
            bbox4.pmax[0].v[3] = boxes[j + 3].pmax.x;

            bbox4.pmax[1].v[0] = boxes[j + 0].pmax.y;
            bbox4.pmax[1].v[1] = boxes[j + 1].pmax.y;
            bbox4.pmax[1].v[2] = boxes[j + 2].pmax.y;
            bbox4.pmax[1].v[3] = boxes[j + 3].pmax.y;

            bbox4.pmax[2].v[0] = boxes[j + 0].pmax.z;
            bbox4.pmax[2].v[1] = boxes[j + 1].pmax.z;
            bbox4.pmax[2].v[2] = boxes[j + 2].pmax.z;
            bbox4.pmax[2].v[3] = boxes[j + 3].pmax.z;
        }
    }

    void TearDown(::benchmark::State &state) override {
        delete rays;
        delete rayInfo;
        delete boxes;
        delete boxes4;
    }

    ray *rays;
    AABB4::RayHitInfo *rayInfo;
    AABB *boxes;
    AABB4 *boxes4;
    Sampler rng;
};

BENCHMARK_DEFINE_F(AABBFixture, SingleAABB)(benchmark::State &st) {
    for (auto _: st) {
        size_t hits = 0;
        for (size_t i = 0; i < NUM_RAYS; ++i) {
            const auto &r      = rays[i];
            const auto &invDir = rayInfo[i].invDir;
            for (size_t j = 0; j < NUM_BOXES; ++j) {
                const auto res = boxes[j].Hit(r, invDir, 0.0f, JTX_INFINITY_F);
                hits += res;
            }
        }
        benchmark::DoNotOptimize(hits);
    }
    st.SetItemsProcessed(st.iterations() * NUM_RAYS * NUM_BOXES);
}
BENCHMARK_REGISTER_F(AABBFixture, SingleAABB)->Unit(benchmark::kMillisecond)->MinWarmUpTime(1);

BENCHMARK_DEFINE_F(AABBFixture, GroupAABB4)(benchmark::State &st) {
    for (auto _: st) {
        size_t hits = 0;
        for (size_t i = 0; i < NUM_RAYS; ++i) {
            const auto &r    = rays[i];
            const auto &info = rayInfo[i];
            for (size_t j = 0; j < NUM_BOXES / 4; ++j) {
                const auto res = boxes4[j].Hit(r, info, 0.0f, JTX_INFINITY_F);
                hits += res.bHit[0] + res.bHit[1] + res.bHit[2] + res.bHit[3];
            }
        }
        benchmark::DoNotOptimize(hits);
    }
    st.SetItemsProcessed(st.iterations() * NUM_RAYS * NUM_BOXES);
}
BENCHMARK_REGISTER_F(AABBFixture, GroupAABB4)->Unit(benchmark::kMillisecond)->MinWarmUpTime(1);

BENCHMARK_MAIN();
