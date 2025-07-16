import common.data;
import common.color;

struct PushConstant {
    float exposure;
    uint32_t tonemapOp;
};
[[vk::push_constant]] PushConstant pc;

[format("rgba32f")]
[[vk::binding(2, 0)]] RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadID : SV_DispatchThreadID) {
    // For testing purposes, we will skip exposure and just apply Reinhard
    uint2 dimensions;
    outputImage.GetDimensions(dimensions.x, dimensions.y);

    uint2 coords = threadID.xy;
    if (coords.x >= dimensions.x || coords.y >= dimensions.y) return;
    float3 hdr = outputImage[coords].rgb;

    float3 tonemapped;
    if (pc.tonemapOp == 0) {
        tonemapped = hdr;
    } else {
        tonemapped = Reinhard(hdr);
    }
    float3 finalColor = LinearToSRGB(tonemapped);

    outputImage[coords] = float4(finalColor, 1.0);
}
