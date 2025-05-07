#pragma once

#include <util/rand.hpp>

namespace jtx {

class rng {
public:
    rng() = default;

    /**
     * Initializes RNG with provided seed
     * @param seed
     */
    explicit rng(const uint32_t seed)
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
    rng(const uint32_t x, const uint32_t y, const uint32_t z) {
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
    vec3 hemisphere(const vec3 &n);

    /**
     * Uniformly samples a point on a unit disc
     * @return point on unit disc
     */
    vec3 unitDisc();

    /**
     * Uniformly samples a point on a unit sphere
     * @return point on unit sphere
     */
    vec3 unitSphere();

private:
    uint32_t m_state = 0;
};

template<typename T>
T rng::uniform() {
    T::unimplemented;
}

template<>
inline float rng::uniform<float>() {
    return (sample() & 0xFFFFFF) / 16777216.0f;
}

template<>
inline vec3 rng::uniform<vec3>() {
    return {uniform<float>(), uniform<float>(), uniform<float>()};
}

template<>
inline vec2 rng::uniform<vec2>() {
    return {uniform<float>(), uniform<float>()};
}

template<typename T>
T rng::range(T min, T max) {
    T::unimplemented;
}

template<>
inline float rng::range<float>(const float min, const float max) {
    return min + (max - min) * uniform<float>();
}

inline vec3 rng::unitVector() {
    const float z = uniform<float>() * 2.0f - 1.0f;
    const float a = uniform<float>() * 2.0f * JTX_PI_F;
    const float r = jtx::sqrt(1.0f - z * z);
    return {r * jtx::cos(a), r * jtx::sin(a), z};
}

inline vec3 rng::hemisphere(const vec3 &n) {
    vec3 p = unitVector();
    return jtx::dot(p, n) > 0 ? p : -p;
}

inline vec3 rng::unitDisc() {
    while (true) {
        auto p = vec3(range<float>(-1, 1), range<float>(-1, 1), 0);
        if (p.lenSqr() < 1) return p;
    }
}

inline vec3 rng::unitSphere() {
    const vec2 u = uniform<vec2>();
    const float z = 1 - 2 * u[0];
    const float a = jtx::safeSqrt(1 - z * z);
    const float phi = TWO_PI * u[1];
    return {jtx::cos(phi) * a, jtx::sin(phi) * a, z};
}

}// namespace jtx