#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "input_structures.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) flat in uint materialId;

layout(location = 0) out vec4 outColor;

void main() {
    // Check if the materialId has a diffuse texture
    GPUMaterialData mat = materialData[materialId];

    vec3 lightDir = normalize(-vec3(-1, -1, -1));
    // Basic lambert diffuse strength
    float d = max(dot(inNormal, lightDir), 0.0f);

    if (mat.diffuseTexture >= 0) {
        outColor = d * vec4(texture(textures[mat.diffuseTexture], inUV).rgb, 1.0);
    } else {
        outColor = d * materialData[materialId].diffuse + vec4(0.1, 0.1, 0.1, .0f);
    }    
}
