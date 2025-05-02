#version 450

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"

layout(push_constant) uniform PushConstants {
    mat4 world;
    mat4 normal;
} pushConstants;

layout(location = 0) out vec2 outUV;

void main() {
    vec3 v = sceneData.positionBuffer.data[gl_VertexIndex];
    gl_Position = sceneData.viewProj * pushConstants.world * vec4(v, 1.0f);

    outUV = sceneData.uvBuffer.data[gl_VertexIndex];
}