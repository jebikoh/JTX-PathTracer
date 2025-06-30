#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(buffer_reference, scalar) readonly buffer Vec3Buffer {
    vec3 data[];
};

layout(buffer_reference, scalar) readonly buffer Vec2Buffer {
    vec2 data[];
};

layout(set = 0, binding = 0) uniform _GPUGlobalUniformData {
    mat4 viewProj;
    mat4 invViewProj;
    vec4 cameraPos;
    vec3 sunDirection;
    float sunIntensity;

    Vec3Buffer vertices;
    Vec3Buffer normals;
    Vec2Buffer uvs;
    Vec3Buffer colors;
};

struct GPUObjectData {
    mat4 world;
    mat4 normal;
    uint material;
};

layout(set = 1, binding = 0) readonly buffer _GPUObjectDataBuffer {
    GPUObjectData objectData[];
};

struct GPUMaterialData {
    vec4 diffuse;
    vec4 ior;
    vec4 k;
    vec4 f0;
    vec4 emission;
    vec4 roughness;
    int diffuseTexture;
};

layout(set = 1, binding = 1) readonly buffer _GPUMaterialDataBuffer {
    GPUMaterialData materialData[];
};

layout(set = 1, binding = 2) uniform sampler2D textures[];
