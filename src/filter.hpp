#pragma once

#include "rt.hpp"

struct FilterSample {
    Vec2f p;
    float weight;
};

class BoxFilter {
public:
    explicit BoxFilter(const Vec2f &radius = Vec2f(0.5f, 0.5f)) : radius_(radius) {}

    Vec2f radius() const { return radius_; }

    float evaluate(Vec2f p) const {
        return (jtx::abs(p.x) <= radius_.x && jtx::abs(p.y) <= radius_.y) ? 1 : 0;
    }

    FilterSample sample(const Vec2f &u) const {
        const Vec2f p(jtx::lerp(u[0], -radius_.x, radius_.x), jtx::lerp(u[1], -radius_.y, radius_.y));
        return {p, 1};
    }

    float integral() const {
        return 2 * radius_.x * 2 * radius_.y;
    }

private:
    Vec2f radius_;
};