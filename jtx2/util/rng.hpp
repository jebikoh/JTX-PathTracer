#pragma once
#include <util/rand.hpp>

namespace jtx {

class RNG {
public:
    RNG() = default;

    /**
     * Initializes RNG with provided seed
     * @param seed
     */
    explicit RNG(const uint32_t seed)
        : m_state(seed) {
        m_state = seed;
    }

    /**
     * Initializes RNG by hashing three 32-bit unsigned integers
     *
     * Utilizes xxhash32
     * @param x X strata
     * @param y Y strata
     * @param z Z strata
     */
    RNG(const uint32_t x, const uint32_t y, const uint32_t z) {
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
    T range(T min, T max);

    void range(float min, float max, vec2 &out);
    void range(float min, float max, vec3 &out);
    void range(float min, float max, vec4 &out);

    /**
     * Generates a random unit vector
     * @return unit vector
     */
    vec3 unitVector();

    /**
     * Uniformly samples a point on a unit hemisphere given a normal
     * @param n normal
     * @return point on unit hemisphere
     */
    vec3 onHemisphere(const vec3 &n);

    /**
     * Samples a point on a unit disc via rejection sampling
     * @return point on unit disc
     */
    vec2 onUnitDisc();

    /**
     * Uniformly samples a point on a unit disc via concentric mapping.
     * Marginally slower than rejection sampling
     * @return uniform point on unit disc
     */
    vec2 onUnitDiscConcentric();

    /**
     * Uniformly samples a point on a unit sphere
     * @return point on unit sphere
     */
    vec3 onUnitSphere();

private:
    uint32_t m_state = 0;
};

template<typename T>
T RNG::uniform() {
    T::unimplemented;
}

template<>
inline float RNG::uniform<float>() {
    return (sample() & 0xFFFFFF) / 16777216.0f;
}

template<>
inline vec3 RNG::uniform<vec3>() {
    return {uniform<float>(), uniform<float>(), uniform<float>()};
}

template<>
inline vec2 RNG::uniform<vec2>() {
    return {uniform<float>(), uniform<float>()};
}

template<typename T>
T RNG::range(T min, T max) {
    T::unimplemented;
}

template<>
inline float RNG::range<float>(const float min, const float max) {
    return min + (max - min) * uniform<float>();
}

inline void RNG::range(const float min, const float max, vec2 &out) {
    out = {range<float>(min, max), range<float>(min, max)};
}

inline void RNG::range(const float min, const float max, vec3 &out) {
    out = {range<float>(min, max), range<float>(min, max), range<float>(min, max)};
}

inline void RNG::range(const float min, const float max, vec4 &out) {
    out = {range<float>(min, max), range<float>(min, max), range<float>(min, max), range<float>(min, max)};
}

inline vec3 RNG::unitVector() {
    const float z = uniform<float>() * 2.0f - 1.0f;
    const float a = uniform<float>() * 2.0f * JTX_PI_F;
    const float r = jtx::sqrt(1.0f - z * z);
    return {r * jtx::cos(a), r * jtx::sin(a), z};
}

inline vec3 RNG::onHemisphere(const vec3 &n) {
    vec3 p = unitVector();
    return jtx::dot(p, n) > 0 ? p : -p;
}

inline vec2 RNG::onUnitDisc() {
    while (true) {
        auto p = vec2(range<float>(-1, 1), range<float>(-1, 1));
        if (p.lenSqr() < 1) return p;
    }
}

// https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#ConcentricSampleDisk
inline vec2 RNG::onUnitDiscConcentric() {
    const vec2 u = 2.0f * uniform<vec2>() - 1.0f;
    if (u.x == 0 && u.y == 0) return {0, 0};

    float theta;
    float r;
    if (jtx::abs(u.x) > jtx::abs(u.y)) {
        r = u.x;
        theta = PI_OVER_4 * (u.y / u.x);
    } else {
        r = u.y;
        theta = PI_OVER_2 - PI_OVER_4 * (u.x / u.y);
    }
    return r * vec2(jtx::cos(theta), jtx::sin(theta));
}

inline vec3 RNG::onUnitSphere() {
    const vec2 u    = uniform<vec2>();
    const float z   = 1 - 2 * u[0];
    const float a   = jtx::safeSqrt(1 - z * z);
    const float phi = TWO_PI * u[1];
    return {jtx::cos(phi) * a, jtx::sin(phi) * a, z};
}

namespace detail {
    inline ray generateRayFromUnitSphere(RNG &rng, const float offsetScale) {
        ray out{};
        out.origin    = rng.onUnitSphere() * offsetScale;
        const vec3 p1 = rng.onUnitSphere() * offsetScale;
        out.dir       = (p1 - out.origin).normalize();
        return out;
    }

    inline ray generateRayToOriginFromUnitSphere(RNG &rng, const float offsetScale) {
        ray out{};
        out.origin = rng.onUnitSphere() * offsetScale;
        out.dir    = (JTX_VEC3_ORIGIN - out.origin).normalize();
        return out;
    }
}// namespace detail

}// namespace jtx