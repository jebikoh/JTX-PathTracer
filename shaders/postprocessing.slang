import common.data;
import common.color;

struct PushConstant {
    float exposure; // Calculated once on the CPU
    uint32_t tonemapOp;
    uint32_t nSamples;
};
[[vk::push_constant]] PushConstant pc;

[format("rgba32f")]
[[vk::binding(1, 0)]] RWTexture2D<float4> accumulationImage;
[format("rgba32f")]
[[vk::binding(2, 0)]] RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadID : SV_DispatchThreadID) {
    uint2 dimensions;
    accumulationImage.GetDimensions(dimensions.x, dimensions.y);

    uint2 coords = threadID.xy;
    if (coords.x >= dimensions.x || coords.y >= dimensions.y) return;

    float3 hdr = accumulationImage[coords].rgb / pc.nSamples;

    // Exposure
    hdr *= pc.exposure;

    // Tonemapping
    float3 tonemapped;
    switch(pc.tonemapOp) {
        case 1:
            tonemapped = Reinhard(hdr);
            break;
        case 2:
            tonemapped = ACESFitted(hdr);
            break;
        case 3:
            tonemapped = AgX(hdr);
            break;
        case 4:
            tonemapped = HableFilmic(hdr);
            break;
        default:
            tonemapped = hdr;
            break;
    }

    // EOTF
    float3 finalColor = LinearToSRGB(tonemapped);

    outputImage[coords] = float4(finalColor, 1.0);
}
