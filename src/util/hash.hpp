#pragma once

#include "../rt.hpp"
#include <cstring>

// All hashing here is taken straight from PBRTv4
// https://github.com/mmp/pbrt-v4/blob/88645ffd6a451bd030d062a55a70a701c58a55d0/src/pbrt/util/hash.h
inline uint64_t mixBits(uint64_t v) {
    v ^= (v >> 31);
    v *= 0x7fb5d329728ea185;
    v ^= (v >> 27);
    v *= 0x81dadef4bc2dd44d;
    v ^= (v >> 33);
    return v;
}

// template<typename... Args>
// inline uint64_t hash(Args... args);

namespace detail {

inline uint64_t murmurHash64A(const unsigned char *key, const size_t len,
                              const uint64_t seed) {
    constexpr uint64_t m = 0xc6a4a7935bd1e995ull;
    constexpr int r      = 47;

    uint64_t h = seed ^ (len * m);

    const unsigned char *end = key + 8 * (len / 8);

    while (key != end) {
        uint64_t k;
        std::memcpy(&k, key, sizeof(uint64_t));
        key += 8;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    switch (len & 7) {
        case 7:
            h ^= static_cast<uint64_t>(key[6]) << 48;
        case 6:
            h ^= static_cast<uint64_t>(key[5]) << 40;
        case 5:
            h ^= static_cast<uint64_t>(key[4]) << 32;
        case 4:
            h ^= static_cast<uint64_t>(key[3]) << 24;
        case 3:
            h ^= static_cast<uint64_t>(key[2]) << 16;
        case 2:
            h ^= static_cast<uint64_t>(key[1]) << 8;
        case 1:
            h ^= static_cast<uint64_t>(key[0]);
            h *= m;
        default:
            break;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

template<typename... Args>
inline void hashCopy(char *buf, Args...);

template<>
inline void hashCopy(char *buf) {}

template<typename T, typename... Args>
inline void hashCopy(char *buf, T v, Args... args) {
    std::memcpy(buf, &v, sizeof(T));
    hashCopy(buf + sizeof(T), args...);
}

}// namespace detail
//
// template<typename... Args>
// inline uint64_t hash(Args... args) {
//     constexpr size_t sz = (sizeof(Args) + ... + 0);
//     constexpr size_t n  = (sz + 7) / 8;
//     uint64_t buf[n];
//     detail::hashCopy(static_cast<char *>(buf), args...);
//     return detail::murmurHash64A(static_cast<const unsigned char *>(buf), sz, 0);
// }

inline uint64_t hash(const int a, const int b, const int c, const int d) {
    constexpr size_t sz = sizeof(int) * 4;
    constexpr auto n  = (sz + 7) / 8 * sizeof(uint64_t);
    char buf[n];
    std::memcpy(buf, &a, sizeof(a));
    std::memcpy(buf + sizeof(a), &b, sizeof(b));
    std::memcpy(buf + sizeof(a) + sizeof(b), &c, sizeof(c));
    std::memcpy(buf + sizeof(a) + sizeof(b) + sizeof(c), &d, sizeof(d));
    return detail::murmurHash64A((const unsigned char *)buf, sz, 0);
}

inline uint64_t hash(Vec2i p, int c, int d) {
    return hash(p.x, p.y, c, d);
}

inline uint64_t hash(int a, int b, int c) {
    constexpr size_t sz = sizeof(int) * 3;
    constexpr auto n  = (sz + 7) / 8 * sizeof(uint64_t);
    char buf[n];
    std::memcpy(buf, &a, sizeof(a));
    std::memcpy(buf + sizeof(a), &b, sizeof(b));
    std::memcpy(buf + sizeof(a) + sizeof(b), &c, sizeof(c));
    return detail::murmurHash64A((const unsigned char *)buf, sz, 0);
}

inline uint64_t hash(Vec2i p, int c) {
    return hash(p.x, p.y, c);
}

inline uint64_t hash(int a, int b) {
    constexpr size_t sz = sizeof(int) * 2;
    constexpr auto n  = (sz + 7) / 8 * sizeof(uint64_t);
    char buf[n];
    std::memcpy(buf, &a, sizeof(a));
    std::memcpy(buf + sizeof(a), &b, sizeof(b));
    return detail::murmurHash64A((const unsigned char *)buf, sz, 0);
}
