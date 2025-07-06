import data_common;

struct PushConstant {
    uint32_t objectId;
};

[[vk::push_constant]] PushConstant pc;

// Binding, Set
[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformData> globalData;
[[vk::binding(0, 1)]] StructuredBuffer<ObjectData> objects;
[[vk::binding(1, 1)]] StructuredBuffer<MaterialData> materials;
[[vk::binding(2, 1)]] Sampler2D textures[];

struct VSInput {
    uint vertexIndex : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
    nointerpolation uint materialId : TEXCOORD1;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
    VSOutput out;
    out.position   = mul(globalData.viewProj, float4(globalData.vertices[input.vertexIndex], 1.0f));
    out.normal     = globalData.normals[input.vertexIndex];
    out.texCoord   = globalData.texCoords[input.vertexIndex];
    out.materialId = objects[pc.objectId].matrialIndex;
    return out;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) {
    MaterialData mat = materials[input.materialId];

    float3 lightDirection = normalize(float3(1, 1, 1));
    float intensity = max(dot(input.normal, lightDirection), 0.2f);

    float4 diffuse;
    if (mat.diffuseTexture >= 0) {
        diffuse = textures[mat.diffuseTexture].Sample(input.texCoord);
    } else {
        diffuse = mat.diffuse;
    }
    return intensity * diffuse;
}