#pragma once
#include "../rt.hpp"

namespace otter {

class Sphere {
public:
    Sphere(Float radius, Float zMin, Float zMax, Float phiMax, const jtx::Transform *objectToRender, const jtx::Transform *renderToObject, bool reverseOrientation);

private:
    Float radius;
    Float zMin, zMax;
    Float thetaZMin, thetaZMax;
    Float phiMax;
    const jtx::Transform *objectToRender, renderToObject;
    bool reverseOrientation, transformSwapsHandedness;
};

}// namespace otter