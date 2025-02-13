#pragma once

#include "../rt.hpp"
#include "../sampling.hpp"

// Plan to implement
// - Point lights
// - Spotlight
// - Directional Lights
// - Uniform/Image Infinite Lights

struct LightSample {
    Vec3 p;
    Vec3 radiance;
    Vec3 wi;
    float pdf;
};

struct LightSampleContext {
    Vec3 p;
    Vec3 n;
};

struct Light {
    enum Type {
        POINT = 0,
    };

    Type type;
    Vec3 position;
    Vec3 intensity;
    float scale;
    float sceneRadius;

    bool sample(const LightSampleContext &ctx, LightSample &sample, const Vec2f &u, bool allowIncompletePDF = false) const {
        switch (type) {
            case POINT:
                sample.p = (position);
                sample.wi = (position - ctx.p).normalize();
                sample.radiance = scale * intensity / jtx::distanceSqr(position, ctx.p);
                sample.pdf = 1;
                return true;
            default:
                return false;
        }
    }

    float pdf(const LightSampleContext &ctx, const Vec3 &wi, bool allowIncompletePDF = false) const {
        switch (type) {
            case POINT:
                return 1;
            default:
                return 0;
        }
    }
};
