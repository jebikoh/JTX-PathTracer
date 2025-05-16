#include <benchmark/benchmark.h>
#include <backends/cpu/bvh.hpp>
#include <jtx.hpp>
#include <util/rng.hpp>
#include <scene/scene_loader.hpp>

using namespace jtx;

static constexpr size_t NUM_RAYS = 1 << 18;

class BVHFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& state) override {
        RNG rng(2002);
        m_rays = new ray[NUM_RAYS];

        for (int i = 0; i < NUM_RAYS; ++i) {
            m_rays[i] = detail::generateRayFromUnitSphere(rng, 2.0f);
        }

        // Load test mesh and create BVH
        jtx::loadScene("assets/f22.obj", m_scene);
        m_bvh.build(m_scene);
    }

    void TearDown(::benchmark::State& state) override {
        m_bvh.destroy();
        delete[] m_rays;
    }

    ray *m_rays;
    Scene m_scene;
    BVH2 m_bvh;
};

BENCHMARK_DEFINE_F(BVHFixture, BVHTraversal)(benchmark::State& st) {
    for (auto _ : st) {
        size_t x = 0;
        for (int i = 0; i < NUM_RAYS; ++i) {
            const auto &r = m_rays[i];
            SurfaceIntersection isect;
            if (m_bvh.closestHit(r, 0, JTX_INFINITY_F, isect)) {
                x += isect.texCoords.x;
            }
        }
        benchmark::DoNotOptimize(x);
    }
    st.SetItemsProcessed(st.iterations() * NUM_RAYS);
}
BENCHMARK_REGISTER_F(BVHFixture, BVHTraversal)->Unit(benchmark::kMillisecond)->Iterations(1000)->MinWarmUpTime(1);

BENCHMARK_MAIN();
