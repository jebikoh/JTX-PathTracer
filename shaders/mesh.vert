#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout(push_constant) uniform _PushConstants {
    uint objectId;
};

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) flat out uint materialId;

void main() {
    vec3 v = vertices.data[gl_VertexIndex];
    gl_Position = viewProj * vec4(v, 1.0f);

    vec3 n = normals.data[gl_VertexIndex];
    outNormal = vec4(n, 0.0f).xyz;

    outUV = uvs.data[gl_VertexIndex];
}
