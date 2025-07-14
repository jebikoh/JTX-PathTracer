#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(buffer_reference, scalar) readonly buffer ivec3buf {
    ivec3 data[];
};

layout(buffer_reference, scalar) readonly buffer vec3buf {
    vec3 data[];
};

layout(buffer_reference, scalar) readonly buffer vec2buf {
    vec2 data[];
};

layout(set = 0, binding = 0) uniform _GPUGlobalUniformData {
    mat4 viewProj;
    mat4 invViewProj;
    vec4 cameraPos;

    vec3buf vertices;
    vec3buf normals;
    vec2buf texCoords;
    vec3buf colors;
    ivec3buf indices;
};

struct GPUObjectData {
    mat4 world;
    mat4 normal;
    uint startIndex;
    uint material;
};

layout(std430, set = 1, binding = 0) readonly buffer _GPUObjectDataBuffer {
    GPUObjectData objectData[];
};

struct GPUMaterialData {
    vec4 diffuse;
    vec4 ior;
    vec4 k;
    vec4 f0;
    vec4 emission;
    vec4 roughness;
    uint type;
    int diffuseTexture;
};

layout(std430, set = 1, binding = 1) readonly buffer _GPUMaterialDataBuffer {
    GPUMaterialData materialData[];
};

layout(set = 1, binding = 2) uniform sampler2D textures[];
