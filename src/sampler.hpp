#pragma once
#include "util/rand.hpp"

// https://github.com/mmp/pbrt-v4/blob/88645ffd6a451bd030d062a55a70a701c58a55d0/src/pbrt/util/math.h#L727
namespace detail {
inline int permutationElement(uint32_t i, uint32_t l, uint32_t p) {
    uint32_t w = l - 1;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    do {
        i ^= p;
        i *= 0xe170893d;
        i ^= p >> 16;
        i ^= (i & w) >> 4;
        i ^= p >> 8;
        i *= 0x0929eb3f;
        i ^= p >> 23;
        i ^= (i & w) >> 1;
        i *= 1 | p >> 27;
        i *= 0x6935fa69;
        i ^= (i & w) >> 11;
        i *= 0x74dcb303;
        i ^= (i & w) >> 2;
        i *= 0x9e501cc3;
        i ^= (i & w) >> 2;
        i *= 0xc860a3df;
        i &= w;
        i ^= i >> 5;
    } while (i >= l);
    return (i + p) % l;
}
}

// Stratified sampler from PBRTv4
// https://github.com/mmp/pbrt-v4/blob/88645ffd6a451bd030d062a55a70a701c58a55d0/src/pbrt/samplers.h#L568
// Other samplers I want to try in the future (in an offline scenario):
// - Halton
// - Sobel
// - PMJ: https://graphics.pixar.com/library/ProgressiveMultiJitteredSampling/paper.pdf
class Sampler {
public:
    Sampler(const int xPixelSamples, const int yPixelSamples, const bool jitter, const int seed = 0)
        : xPixelSamples_(xPixelSamples),
          yPixelSamples_(yPixelSamples),
          seed_(seed),
          jitter_(jitter){};

    int samplesPerPixel() const { return xPixelSamples_ * yPixelSamples_; }

    void init(const Vec2i &p, const int index, const int dim) {
        pixel_ = p;
        sampleIndex_ = index;
        dimension_ = dim;
        rng_.setSequence(hash(p, seed_));
        rng_.advance(sampleIndex_ * 65536ull + dimension_);
    }

    float get1D() {
        const uint64_t h = hash(pixel_, dimension_, seed_);
        const int stratum = detail::permutationElement(sampleIndex_, samplesPerPixel(), h);

        ++dimension_;
        const float delta = jitter_ ? rng_.sampleFP() : 0.5f;
        return (stratum + delta) / samplesPerPixel();
    }

    Vec2f get2D() {
        const uint64_t h = hash(pixel_, dimension_, seed_);
        const int stratum = detail::permutationElement(sampleIndex_, samplesPerPixel(), h);

        dimension_ += 2;
        const int x = stratum % xPixelSamples_;
        const int y = stratum / xPixelSamples_;

        const float dx = jitter_ ? rng_.sampleFP() : 0.5f;
        const float dy = jitter_ ? rng_.sampleFP() : 0.5f;

        return {(x + dx) / xPixelSamples_, (y + dy) / yPixelSamples_};
    }

private:
    int xPixelSamples_, yPixelSamples_;
    int seed_;
    bool jitter_;
    PCG32 rng_;
    Vec2i pixel_;
    int sampleIndex_ = 0;
    int dimension_   = 0;
};