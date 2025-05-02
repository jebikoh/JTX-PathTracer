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

layout (set = 1, binding = 0) uniform MaterialData {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D ambientTex;
layout (set = 1, binding = 2) uniform sampler2D diffuseTex;
layout (set = 1, binding = 3) uniform sampler2D specularTex;
