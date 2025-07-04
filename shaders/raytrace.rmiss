#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
    prd.hitValue = vec3(0.0, 0.1, 0.3);
}