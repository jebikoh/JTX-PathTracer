#pragma once
#include <util/rand.hpp>

namespace jtx {

/**
 * Class is responsible for sampling stratified random variables
 */
class Sampler {
public:
    Sampler() = default;

    /**
     * Initializes sampler with provided seed
     * @param seed
     */
    explicit Sampler(const uint32_t seed)
        : m_state(seed) {
        m_state = seed;
    }

    /**
     * Initializes sampler by hashing three 32-bit unsigned integers
     *
     * Utilizes xxhash32
     * @param x X strata
     * @param y Y strata
     * @param z Z strata
     */
    Sampler(const uint32_t x, const uint32_t y, const uint32_t z) {
        m_state = xxhash32({x, y, z});
    }

    /**
     * Samples a 32-bit unsigned integer
     *
     * Uses a RXS-M-XS PCG
     * @return random 32-bit unsigned integer
     */
    uint32_t sample() {
        return pcg(m_state);
    }

    /**
     * Samples a 32-bit unsigned integer in the range [0, range)
     *
     * Uses Lemire's method:
     *  - https://www.pcg-random.org/posts/bounded-rands.html
     *  - https://lemire.me/blog/2019/06/06/nearly-divisionless-random-integer-generation-on-various-systems/
     *  - https://arxiv.org/abs/1805.10941
     * @param range upper bound
     * @return random 32-bit unsigned integer on [0, range)
     */
    uint32_t sample(const int range) {
        const uint32_t t = -range % range;

        uint64_t m;
        uint32_t l;
        do {
            const uint32_t x = sample();
            m                = static_cast<uint64_t>(x) * static_cast<uint64_t>(range);
            l                = static_cast<uint32_t>(m);
        } while (l < t);
        return m >> 32;
    }

    /**
     * Generates a uniform random variable on [0, 1) of type T
     * @tparam T float type (float, vecX)
     * @return random variable on [0, 1)
     */
    template<typename T>
    T uniform();

    /**
     * Generates a random variable of type T within the provided range
     * @tparam T random variable type
     * @param min lower bound
     * @param max upper bound
     * @return
     */
    template<typename T>
    T uniform(float min, float max);
private:
    uint32_t m_state = 0;
};

#pragma region Uniform
template<typename T>
T Sampler::uniform() {
    return T::unimplemented;
}

template<>
inline float Sampler::uniform<float>() {
    return (sample() & 0xFFFFFF) / 16777216.0f;
}

template<>
inline vec2 Sampler::uniform<vec2>() {
    return {uniform<float>(), uniform<float>()};
}

template<>
inline vec3 Sampler::uniform<vec3>() {
    return {uniform<float>(), uniform<float>(), uniform<float>()};
}
#pragma endregion

#pragma region Range
template<typename T>
T Sampler::uniform(float min, float max) {
    return T::unimplemented;
}

template<>
inline float Sampler::uniform<float>(const float min, const float max) {
    return min + (max - min) * uniform<float>();
}

template<>
inline vec2 Sampler::uniform<vec2>(const float min, const float max) {
    return {uniform<float>(min, max), uniform<float>(min, max)};
}

template<>
inline vec3 Sampler::uniform<vec3>(const float min, const float max) {
    return {uniform<float>(min, max), uniform<float>(min, max), uniform<float>(min, max)};
}

template<>
inline vec4 Sampler::uniform<vec4>(const float min, const float max) {
    return {uniform<float>(min, max), uniform<float>(min, max), uniform<float>(min, max), uniform<float>(min, max)};
}

#pragma endregion

#pragma region Sampling Functions
// These are kept separate to allow sampling routines to preserve stratification
// and sampling patterns across different distributions.
// Most of these are taken from PBR 4d
JTX_FORCE_INLINE vec3 sampleUniformSphere(const vec2 &s) {
    const float z = 1 - 2 * s.x;
    const float a = jtx::safeSqrt(1.0f - z * z);
    const float phi = TWO_PI * s.y;
    return {jtx::cos(phi) * a, jtx::sin(phi) * a, z};
}

JTX_FORCE_INLINE float uniformUniformSpherePDF() {
    return INV_4_PI;
}

JTX_FORCE_INLINE vec2 sampleUniformDiskConcentric(const vec2 &s) {
    const vec2 offset = 2.0f * s - vec2(1.0f, 1.0f);
    if (offset.x == 0 && offset.y == 0) return {0.0f, 0.0f};

    float r, theta;
    if (jtx::abs(offset.x) > jtx::abs(offset.y)) {
        // X is dominant axis
        r = offset.x;
        theta = PI_OVER_4 * (offset.y / offset.x);
    } else {
        // Y is dominant axis
        r = offset.y;
        theta = PI_OVER_2 - PI_OVER_4 * (offset.x / offset.y);
    }

    return {r * jtx::cos(theta), r * jtx::sin(theta)};
}

JTX_FORCE_INLINE vec3 sampleUniformHemisphere(const vec2 &s) {
    const float sinTheta = jtx::safeSqrt(1 - s.x * s.x);
    const float phi = 2 * JTX_PI_F * s.y;
    return {jtx::cos(phi) * sinTheta, jtx::sin(phi) * sinTheta, s.x};
}

JTX_FORCE_INLINE float uniformHemispherePDF() {
    return INV_TWO_PI;
}

JTX_FORCE_INLINE vec3 sampleCosineHemisphere(const vec2 &s) {
    const auto disk = sampleUniformDiskConcentric(s);
    return {disk.x, disk.y, jtx::safeSqrt(1 - disk.x * disk.x - disk.y * disk.y)};
}

JTX_FORCE_INLINE float cosineHemispherePDF(const float cosTheta) {
    return cosTheta * INV_PI;
}

namespace detail {
    inline ray generateRayFromUnitSphere(Sampler &rng, const float offsetScale) {
        ray out{};

        out.origin    = sampleUniformSphere(rng.uniform<vec2>()) * offsetScale;
        const vec3 p1 = sampleUniformSphere(rng.uniform<vec2>()) * offsetScale;
        out.dir       = (p1 - out.origin).normalize();
        return out;
    }

    inline ray generateRayToOriginFromUnitSphere(Sampler &rng, const float offsetScale) {
        ray out{};
        out.origin = sampleUniformSphere(rng.uniform<vec2>()) * offsetScale;
        out.dir    = (JTX_VEC3_ORIGIN - out.origin).normalize();
        return out;
    }
}// namespace detail
#pragma endregion

}// namespace jtx