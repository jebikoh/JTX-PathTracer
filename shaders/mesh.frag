#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(-vec3(-1, -1, -1));
    // Basic lambert diffuse strength
    float d = max(dot(inNormal, lightDir), 0.0f);
    // outColor = d * vec4(texture(diffuseTex, inUV).rgb, 1.0);
    outColor = d * materialData.diffuse + vec4(0.1, 0.1, 0.1, .0f);
}
