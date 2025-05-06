#pragma once

#include "jtx.hpp"

constexpr float TWO_PI = jtx::JTX_PI_F * 2.0f;

inline vec3 sampleUniformSphere(vec2 u) {
  const float z = 1 - 2 * u[1];
  const float a = jtx::safeSqrt(1 - z * z);
  const float phi = TWO_PI * u[1];
  return {jtx::cos(phi) * a, jtx::sin(phi) * a, z};
}