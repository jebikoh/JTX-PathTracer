#pragma once
#include "../rt.hpp"

struct Interval {
    Float min, max;

    Interval() : min(INF), max(-INF) {}

    Interval(Float min, Float max) : min(min), max(max) {}

    [[nodiscard]]
    Float size() const {
        return max - min;
    }

    [[nodiscard]]
    bool contains(const Float x) const {
        return min <= x && x <= max;
    }

    [[nodiscard]]
    bool surrounds(const Float x) const {
        return min < x && x < max;
    }

    [[nodiscard]]
    Float clamp(Float x) const {
        return jtx::clamp(x, min, max);
    }

    [[nodiscard]]
    Interval expand(Float delta) const {
        const auto padding = delta / 2;
        return {min - padding, max + padding};
    }

    static const Interval EMPTY;
    static const Interval UNIVERSE;
};
