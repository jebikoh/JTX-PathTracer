#version 450

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"

layout(push_constant) uniform PushConstants {
    mat4 world;
    mat4 normal;
} pushConstants;

layout(location = 0) out vec3 outColor;

void main() {
    vec3 v = sceneData.positionBuffer.data[gl_VertexIndex];
    vec3 n = sceneData.normalBuffer.data[gl_VertexIndex];
    gl_Position = sceneData.viewProj * pushConstants.world * vec4(v, 1.0f);
    // outColor = vec3(1.0, 0.0, 0.0); // output color always to red for debugging
    outColor = n;
}