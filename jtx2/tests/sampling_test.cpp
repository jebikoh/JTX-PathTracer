#include <gtest/gtest.h>
#include <util/sampling.hpp>
#include <vector>

namespace jtx {

// Global test settings
constexpr uint32_t JTX_TEST_EPSILON = 1e-5;
constexpr uint32_t JTX_N = 2 << 16;
constexpr float JTX_99_Z            = 2.576f; // Z-score for 99% confidence interval
constexpr float JTX_95_Z            = 1.96f;   // Z-score for 95% confidence interval

TEST(Sampler, SameSeedProducesSameDistribution) {
    Sampler sampler1{101};
    Sampler sampler2{101};
    EXPECT_EQ(sampler1.Sample(), sampler2.Sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 102, 0);
    EXPECT_EQ(sampler1.Sample(), sampler2.Sample());
}

TEST(Sampler, SeedHashChangesDistribution) {
    Sampler sampler1(0);
    Sampler sampler2(1);
    EXPECT_NE(sampler1.Sample(), sampler2.Sample());

    // Strata should be different
    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 102, 1);
    EXPECT_NE(sampler1.Sample(), sampler2.Sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 103, 0);
    EXPECT_NE(sampler1.Sample(), sampler2.Sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(2431, 102, 0);
    EXPECT_NE(sampler1.Sample(), sampler2.Sample());
}

// Below are statistical tests for the Sampler class
// Each test will check two basic things:
// 1. All values are within the correct range
// 2. Test that first moment lies within a reasonable confidence band
//
// In the future, may add goodness-of-fit tests like X^2 or KS

// Discrete uniform distribution
TEST(Sampler, UniformU32) {
    constexpr uint32_t range = 128; // Sample [0, RANGE)
    Sampler sampler(20252505);

    constexpr float mu = static_cast<float>(range - 1) / 2.0f;
    constexpr float var = range * range / 12.0f;
    const float epsMu = 2 * jtx::Sqrt(var / JTX_N);

    uint64_t sum = 0;
    for (uint32_t i = 0; i < JTX_N; ++i) {
        const uint32_t sample = sampler.Sample(range);
        EXPECT_GE(sample, 0);
        EXPECT_LT(sample, range);
        sum += sampler.Sample(range);
    }
    const float sampleMean = static_cast<float>(sum) / static_cast<float>(JTX_N);

    EXPECT_LT(jtx::abs(sampleMean - mu), epsMu);
}

// Continuous uniform distribution
TEST(Sampler, UniformFP32) {
    Sampler sampler(20252605);

    constexpr float mu = 0.5f;
    constexpr float var = 1.0f / 12.0f;
    const float epsMu = 2 * jtx::Sqrt(var / JTX_N);

    float sum = 0;
    for (uint32_t i = 0; i < JTX_N; ++i) {
        const float sample = sampler.Uniform<float>();
        EXPECT_GE(sample, 0.0f);
        EXPECT_LE(sample, 1.0f);
        sum += sample;
    }
    const float sampleMean = sum / static_cast<float>(JTX_N);

    EXPECT_LT(jtx::abs(sampleMean - mu), epsMu);
}

TEST(Sampler, UniformFP32Range) {
    Sampler sampler(20252705);

    constexpr float rmin = -3.77f;
    constexpr float rmax = 23.32f;

    constexpr float mu = (rmin + rmax) * 0.5f;
    const float var = jtx::Sqr((rmax - rmin)) * (1.0f / 12.0f);
    const float epsMu   = 2 * jtx::Sqrt(var / JTX_N);

    float sum = 0;
    for (uint32_t i = 0; i < JTX_N; ++i) {
        const float sample = sampler.Uniform<float>(rmin, rmax);
        EXPECT_GE(sample, rmin);
        EXPECT_LE(sample, rmax);
        sum += sample;
    }
    const float sampleMean = sum / static_cast<float>(JTX_N);

    EXPECT_LT(jtx::abs(sampleMean - mu), epsMu);
}

TEST(Sampler, UniformSphere) {
    Sampler sampler(20252805);

    const auto mu = vec3(0.0f, 0.0f, 0.0f);
    const
}

}// namespace jtx
