#pragma once
#include "rt.hpp"

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

    static const Interval EMPTY, UNIVERSE;
};

const Interval Interval::EMPTY    = Interval(INF, -INF);
const Interval Interval::UNIVERSE = Interval(-INF, INF);

[[nodiscard]]