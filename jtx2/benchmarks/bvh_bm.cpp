#include <../engine/jtx/cpu/bvh.hpp>
#include <benchmark/benchmark.h>
#include <jtx.hpp>
#include <scene/scene_loader.hpp>
#include <util/rng.hpp>

using namespace jtx;

static constexpr size_t NUM_RAYS = 1 << 18;

class BVHFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& state) override {
        Sampler rng(2002);
        m_rays = new ray[NUM_RAYS];

        for (int i = 0; i < NUM_RAYS; ++i) {
            m_rays[i] = detail::generateRayFromUnitSphere(rng, 2.0f);
        }

        // Load test mesh and create BVH
        jtx::LoadScene("assets/f22.obj", m_scene);
        m_bvh.Build(m_scene);
        m_bvhEmbree.Build(m_scene);
    }

    void TearDown(::benchmark::State& state) override {
        m_bvh.Destroy();
        m_bvhEmbree.Destroy();
        delete[] m_rays;
    }

    ray *m_rays;
    Scene m_scene;
    BVH2 m_bvh;

#ifdef JTX_USE_EMBREE
    BVHEmbree m_bvhEmbree;
#endif
};

BENCHMARK_DEFINE_F(BVHFixture, BVHTraversal)(benchmark::State& st) {
    for (auto _ : st) {
        size_t x = 0;
        for (int i = 0; i < NUM_RAYS; ++i) {
            const auto &r = m_rays[i];
            TriangleIntersection isect;
            if (m_bvh.ClosestHit(r, 0, JTX_INFINITY_F, isect)) {
                x += isect.u;
            }
        }
        benchmark::DoNotOptimize(x);
    }
    st.SetItemsProcessed(st.iterations() * NUM_RAYS);
}
BENCHMARK_REGISTER_F(BVHFixture, BVHTraversal)->Unit(benchmark::kMillisecond)->Iterations(1000)->MinWarmUpTime(1);

#ifdef JTX_USE_EMBREE

BENCHMARK_DEFINE_F(BVHFixture, BVHEmbreeTraversal)(benchmark::State &st) {
    for (auto _ : st) {
        size_t x = 0;
        for (int i = 0; i < NUM_RAYS; ++i) {
            const auto &r = m_rays[i];
            TriangleIntersection isect;
            if (m_bvhEmbree.ClosestHit(r, 0, JTX_INFINITY_F, isect)) {
                x += isect.u;
            }
        }
        benchmark::DoNotOptimize(x);
    }
    st.SetItemsProcessed(st.iterations() * NUM_RAYS);
}
BENCHMARK_REGISTER_F(BVHFixture, BVHEmbreeTraversal)->Unit(benchmark::kMillisecond)->Iterations(1000)->MinWarmUpTime(1);

#endif

BENCHMARK_MAIN();
