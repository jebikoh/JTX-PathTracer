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
     * @param x X strata
     * @param y Y strata
     * @param z Z strata
     */
    RNG(const uint32_t x, const uint32_t y, const uint32_t z) {
        m_state = xxhash32({x, y, z});
    }

    /**
     * Samples a 32-bit unsigned integer
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

private:
    uint32_t m_state = 0;
};

template<typename T>
T uniform() {
    return T::unimplemented;
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

}// namespace jtx