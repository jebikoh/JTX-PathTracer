import common.data;

struct PushConstant {
    float4 wireframeColor;
};

[[vk::push_constant]] PushConstant pc;
[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformData> globalData;

struct VSInput {
    uint vertexIndex : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
    VSOutput out;
    out.position = mul(globalData.viewProj, float4(globalData.vertices[input.vertexIndex], 1.0f));
    return out;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) {
    return pc.wireframeColor;
}