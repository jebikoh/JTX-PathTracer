#pragma once

#include "../rt.hpp"

// Plan to implement
// - Point lights
// - Spotlight
// - Directional Lights
// - Uniform/Image Infinite Lights

struct LightSample {
    Vec3 radiance;
    Vec3 wi;
    float pdf;
};

struct LightSampleContext {
    Vec3 p;
    Vec3 n;
    Vec3 sn;
};

struct Light {
    enum Type {
        POINT = 0,
        SPOT = 1,
        DIRECTIONAL = 2,
        INFINITE = 3
    };

    Type type;
    Vec3 position;
    Vec3 intensity;
    float scale;
};

// TODO: move to source file
inline bool sampleLight(const Light &light, const LightSampleContext &ctx, LightSample &sample) {
    switch (light.type) {
        case Light::POINT:
            sample.wi = (light.position - ctx.p).normalize();
            sample.radiance = light.scale * light.intensity / jtx::distanceSqr(light.position, ctx.p);
            sample.pdf = 1;
            return true;
        default:
            return false;
    }
}

// TODO: move to source file
inline float pdfLight(const Light &light, const LightSampleContext &ctx, const Vec3 &wi) {
    switch (light.type) {
        case Light::POINT:
            return 1;
        default:
            return 0;
    }
}