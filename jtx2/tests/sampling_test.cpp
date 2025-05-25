#include <gtest/gtest.h>
#include <util/sampling.hpp>

namespace jtx {

// Global test settings
constexpr uint32_t JTX_TEST_EPSILON = 1e-5;
constexpr uint32_t JTX_TEST_SAMPLES = 2 << 10;

TEST(Sampler, SameSeedProducesSameDistribution) {
    Sampler sampler1{101};
    Sampler sampler2{101};
    EXPECT_EQ(sampler1.sample(), sampler2.sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 102, 0);
    EXPECT_EQ(sampler1.sample(), sampler2.sample());
}

TEST(Sampler, SeedHashChangesDistribution) {
    Sampler sampler1(0);
    Sampler sampler2(1);
    EXPECT_NE(sampler1.sample(), sampler2.sample());

    // Strata should be different
    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 102, 1);
    EXPECT_NE(sampler1.sample(), sampler2.sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(101, 103, 0);
    EXPECT_NE(sampler1.sample(), sampler2.sample());

    sampler1 = Sampler(101, 102, 0);
    sampler2 = Sampler(2431, 102, 0);
    EXPECT_NE(sampler1.sample(), sampler2.sample());
}

// TEST(Sampler, SampleUint32Uniform) {
//     // Test settings
//     constexpr uint32_t SEED       = 101;
//     constexpr uint32_t RANGE      = 32;
//
//     constexpr float true_mean = (RANGE - 1) / 2.0f;
//     constexpr float true_variance  = (RANGE * RANGE - 1) / 12.0f;
//
//     constexpr float epsMean =  3 * jtx::sqrt(true_variance / JTX_TEST_SAMPLES);
// }

}// namespace jtx
