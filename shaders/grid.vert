// Based on: https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/
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

vec3 gridVertices[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

layout(location = 0) out vec3 outNearPoint;
layout(location = 1) out vec3 outFarPoint;
layout(location = 2) out vec3 outCameraPos;
layout(location = 3) out mat4 outViewProj;

vec3 Unproject(vec3 p, mat4 viweProjInv) {
    vec4 p2 = viweProjInv * vec4(p, 1.0);
    return p2.xyz / p2.w;
}

void main() {
    // Grid coordinates are written given in NDC
    vec3 p = gridVertices[gl_VertexIndex].xyz;

    // For shading calculations, we need to know the near and far point for each fragment.
    // This is simply done by appliying the inverse view-projection transform and persp. division
    mat4 invViewProj = inverse(sceneData.viewProj); // TODO: calculate on CPU
    outNearPoint = Unproject(vec3(p.xy, 0.0), invViewProj);
    outFarPoint = Unproject(vec3(p.xy, 1.0), invViewProj);

    // We need to pass this so we can calculate depth in the fragment shader
    outViewProj = sceneData.viewProj;

    // And we pass this so we can apply a distance-based fade effect in the fragment shader
    outCameraPos = sceneData.cameraPos.xyz;

    gl_Position = vec4(p, 1.0);
}
