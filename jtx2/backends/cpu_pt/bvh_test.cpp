#include <gtest/gtest.h>

#include <functional>

#include <backends/cpu_pt/bvh.hpp>
#include <scene/scene_loader.hpp>
#include <util/rng.hpp>

namespace jtx {

#ifdef JTX_BUILD_TESTS

const std::string JTX_BVH_TEST_MESH_PATH  = "assets/f22.obj";
constexpr int JTX_BVH_TEST_NUM_SAMPLES    = 10000;
constexpr int JTX_BVH_TEST_RNG_SEED       = 1234567;
constexpr float JTX_BVH_TEST_EPSILON      = 1e-5;
constexpr float JTX_BVH_TEST_OFFSET_SCALE = 5.0f;

void validateBVH(
        const BVHEmbree &bvhEmbree,
        const std::function<void(const ray &, float &)> &isectFn,
        const std::function<ray(Sampler &)> &rayGen,
        const uint32_t seed  = JTX_BVH_TEST_RNG_SEED,
        const int numSamples = JTX_BVH_TEST_NUM_SAMPLES,
        const float epsilon  = JTX_BVH_TEST_EPSILON) {
    Sampler sampler{seed};
    float mse        = 0.0f;
    uint32_t trueHit = 0;

    for (int i = 0; i < numSamples; ++i) {
        // Generate point
        ray r = rayGen(sampler);


        // Test embree
        float tEmbree = -1.0f;
        TriangleIntersection isect;
        if (bvhEmbree.closestHit(r, 0.0f, JTX_INFINITY_F, isect)) {
            tEmbree = isect.t;
        }

        float tJtx    = -1.0f;
        isectFn(r, tJtx);

        bool bEmbreeHit = tEmbree >= 0.0f;
        bool bJtxHit    = tJtx >= 0.0f;
        EXPECT_EQ(bEmbreeHit, bJtxHit);

        if (bEmbreeHit && bJtxHit) {
            const float error = tEmbree - tJtx;
            mse += error * error;
            trueHit++;
        }
    }

    mse /= static_cast<float>(trueHit);
    EXPECT_LT(mse, epsilon);
    LOG_INFO(TEST, "Final MSE: {} ({} samples)", mse, trueHit);
}

class BVH2Test : public testing::Test {
protected:
    BVH2Test() {
        ASSERT(jtx::loadScene(JTX_BVH_TEST_MESH_PATH, m_scene));
        m_bvh.build(m_scene);
        m_bvhEmbree.build(m_scene);
    }

    ~BVH2Test() override {
        m_bvhEmbree.destroy();
        m_bvh.destroy();
    }

    Scene m_scene;

    BVH2 m_bvh;
    BVHEmbree m_bvhEmbree;

    const std::function<void(const ray &, float &)> m_isectFn = [&](const ray &r, float &t) {
        TriangleIntersection isect;
        if (m_bvh.closestHit(r, 0, JTX_INFINITY_F, isect)) {
            t = isect.t;
        }
    };
};

TEST_F(BVH2Test, BVH2RaysToOrigin) {
    const auto rayGen = [](Sampler &rng) {
        return detail::generateRayToOriginFromUnitSphere(rng, JTX_BVH_TEST_OFFSET_SCALE);
    };
    validateBVH(m_bvhEmbree, m_isectFn, rayGen);
}

TEST_F(BVH2Test, BVH2RandomRays) {
    const auto rayGen = [](Sampler &rng) {
        return detail::generateRayFromUnitSphere(rng, JTX_BVH_TEST_OFFSET_SCALE);
    };
    validateBVH(m_bvhEmbree, m_isectFn, rayGen);
}

#endif

}// namespace jtx
