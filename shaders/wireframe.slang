import common.data;

struct PushConstant {
    float4 wireframeColor;
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
    float4 color : COLOR;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
    VSOutput out;
    out.position = mul(globalData.viewProj, float4(globalData.vertices[input.vertexIndex], 1.0f));
    out.color    = pc.wireframeColor; 
    return out;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) {
    return input.color;
}