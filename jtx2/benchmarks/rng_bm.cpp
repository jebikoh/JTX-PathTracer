#include <benchmark/benchmark.h>
#include <util/rng.hpp>

using namespace jtx;

static constexpr uint32_t NUM_SAMPLES = 1 << 20;
static constexpr uint32_t SEED    = 1234567;

class RNGFixture : public benchmark::Fixture {
public:
    Sampler a{SEED};
    Sampler b{SEED};
};

/**
 * BENCHMARK RESULTS: 1.5M samples/sec
 * Captured on: M3 Pro 32GB
 *
 * BENCHMARK RESULTS: 1.36957M samples/sec
 * Captured on: Ryzen 7 5800X 3.80GHz 32GB
 */
BENCHMARK_DEFINE_F(RNGFixture, UnitDiscRejectionSampling)(benchmark::State &st) {
    for (auto _ : st) {
        vec2 sum;
        for (size_t i = 0; i < NUM_SAMPLES; i++) {
            sum += a.uniformUnitDisc();
        }
        benchmark::DoNotOptimize(sum);
    }
    st.SetItemsProcessed(NUM_SAMPLES);
}
BENCHMARK_REGISTER_F(RNGFixture, UnitDiscRejectionSampling)->Unit(benchmark::kMillisecond)->MinWarmUpTime(1);

/**
 * BENCHMARK RESULTS: 1.45M samples/sec
 * Captured on: M3 Pro 32GB
 *
 * BENCHMARK RESULTS: 1.56067M samples/sec
 * Captured on: Ryzen 7 5800X 3.80GHz 32GB
 */
BENCHMARK_DEFINE_F(RNGFixture, UnitDiscConcentricSampling)(benchmark::State &st) {
    for (auto _ : st) {
        vec2 sum;
        for (size_t i = 0; i < NUM_SAMPLES; i++) {
            sum += a.uniformUnitDiscConcentric();
        }
        benchmark::DoNotOptimize(sum);
    }
    st.SetItemsProcessed(NUM_SAMPLES);
}
BENCHMARK_REGISTER_F(RNGFixture, UnitDiscConcentricSampling)->Unit(benchmark::kMillisecond)->MinWarmUpTime(1);

BENCHMARK_MAIN();
