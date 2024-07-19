#pragma once

#include <jtxlib/math/constants.hpp>

class Interval {
public:
    double min, max;

    Interval() : min(+jtx::INFINITY_D), max(jtx::NEG_INFINITY_D) {}
    Interval(double min, double max) : min(min), max(max) {}

    [[nodiscard]] double size() const { return max - min; }

    [[nodiscard]] bool contains(double x) const { return x >= min && x <= max; }

    [[nodiscard]] bool surrounds(double x) const { return x > min && x < max; }

    static const Interval empty, universe;
};

const Interval Interval::empty = Interval(+jtx::INFINITY_D, jtx::NEG_INFINITY_D);
const Interval Interval::universe = Interval(jtx::NEG_INFINITY_D, jtx::INFINITY_D);
