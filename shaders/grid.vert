#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(buffer_reference, scalar) readonly buffer Vec3Buffer {
    vec3 data[];
};

layout(buffer_reference, scalar) readonly buffer Vec2Buffer {
    vec2 data[];
};

layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 cameraPos;
    Vec3Buffer positionBuffer;
    Vec3Buffer normalBuffer;
    Vec2Buffer uvBuffer;
    Vec3Buffer colorBuffer;
} sceneData;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outCameraPos;
layout(location = 2) out mat4 outInvViewProj;
layout(location = 6) out mat4 outViewProj;

const vec3 positions[3] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3( 3.0, -1.0, 0.0),
    vec3(-1.0,  3.0, 0.0)
);

void main() {
    vec3 pos = positions[gl_VertexIndex].xyz;
    gl_Position = vec4(pos, 1.0);

    outUV = (pos.xy + 1.0) * 0.5;
    outInvViewProj = inverse(sceneData.viewProj); // TODO: move this to CPU
    outViewProj = sceneData.viewProj;
    outCameraPos = sceneData.cameraPos.xyz;
}
