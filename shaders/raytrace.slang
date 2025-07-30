import common.data;
import common.math;
import common.bxdf;
import common.random;
import common.color;
import common.constants;

struct PushConstant {
    matrix<float, 4, 4> invProj;
    matrix<float, 4, 4> invView;
    uint32_t currentSample;
    uint32_t nSamples;
    float directClamping;
    float indirectClamping;
};
[[vk::push_constant]] PushConstant pc;

[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformData> globalData;
[format("rgba32f")]
[[vk::binding(1, 0)]] RWTexture2D<float4> accumulationImage;
[format("rgba32f")]
[[vk::binding(2, 0)]] RWTexture2D<float4> outputImage;

[[vk::binding(0, 1)]] StructuredBuffer<ObjectData> objects;
[[vk::binding(1, 1)]] StructuredBuffer<MaterialData> materials;
[[vk::binding(2, 1)]] Sampler2D textures[];
[[vk::binding(3, 1)]] RaytracingAccelerationStructure topLevelAS;

struct Attributes {
    float2 uv;
};

struct Payload {
    bool bMiss;

    float3 emission;
    float3 f;
    float3 wi;
    float pdf;

    float3 position;
    float3 normal;

    float s0;
    float2 s1;
};

[shader("raygeneration")]
void rayGenMain() {
    uint3 launchID   = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    float3 accumulatedRadiance = {0, 0, 0};
    uint32_t start = pc.currentSample;
    uint32_t stop  = pc.currentSample + pc.nSamples;

    bool bSampleHDRI = globalData.envmapType == 1 && globalData.envmapTexture >= 0;
    bool bClampIndirect = pc.indirectClamping > 0.0;

    for (uint32_t s = start; s < stop; s++) {
        RNG rng = RNG(launchID.x, launchID.y, s);
        
        // Camera ray
        const float2 jitter = rng.UniformFloat2();

        const float2 pixelCenter = float2(launchID.xy) + jitter;
        const float2 inUV        = pixelCenter / float2(launchSize.xy);
        const float2 d           = inUV * 2.0 - 1.0;

        RayDesc ray;
        float3 target    = mul(pc.invProj, float4(d.x, d.y, 1, 1)).xyz;
        ray.Direction = mul(pc.invView, float4(normalize(target), 0)).xyz;
        ray.Origin = mul(pc.invView, float4(0, 0, 0, 1)).xyz;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;

        float3 radiance = float3(0, 0, 0);
        float3 beta     = float3(1, 1, 1);
        int depth = 0;
        
        Payload prd;
        while(true) {
            prd.bMiss  = true;
            prd.wi     = -ray.Direction;
            // TODO: just pass the seed
            prd.s0     = rng.UniformFloat();
            prd.s1     = rng.UniformFloat2();

            TraceRay(topLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, prd);

            if (prd.bMiss) {
                float3 L;
                if (bSampleHDRI) {
                    float3 r = ray.Direction;
                    float theta = acos(clamp(r.y, -1.0, 1.0)) + globalData.verticalOffset;
                    float phi   = atan2(r.z, r.x) + globalData.horizontalOffset;

                    const float u = phi * INV_2_PI + 0.5f;
                    const float v = theta * INV_PI;
                    L = beta * textures[globalData.envmapTexture].SampleLevel(float2(u, v), 0.0).rgb;
                } else {
                    L = beta * globalData.envmapColor * globalData.envmapIntensity;
                }
                
                if (depth > 0 && bClampIndirect) {
                    L = min(L, pc.indirectClamping);
                }
                radiance += L;

                break;
            }

            float3 L = beta * prd.emission;
            if (depth > 0 && bClampIndirect) {
                L = min(L, pc.indirectClamping);
            }
            radiance += L;

            if (depth++ == 32) break;

            if (prd.pdf == 0) break;

            beta *= prd.f * abs(dot(prd.wi, prd.normal)) / prd.pdf;

            if (isinf(beta.x) || isnan(beta.x)) break;

            ray.Origin    = prd.position + prd.wi * 0.001;
            ray.Direction = prd.wi;
        }

        accumulatedRadiance += radiance;
    }

    float4 totalAccumulation = float4(accumulatedRadiance, 1.0);
    if (pc.currentSample == 0) {
        accumulationImage[int2(launchID.xy)] = totalAccumulation;
    } else {
        totalAccumulation += accumulationImage[int2(launchID.xy)];
        accumulationImage[int2(launchID.xy)] = totalAccumulation;
    }
}

[shader("closesthit")]
void closestHitMain(inout Payload prd, in Attributes attribs) {
    prd.bMiss = false;

    ObjectData obj = objects[InstanceIndex()];
    MaterialData mat = materials[obj.materialIndex];

    const float3 b = float3(1.0 - attribs.uv.x - attribs.uv.y, attribs.uv.x, attribs.uv.y);

    uint3 idx = globalData.indices[obj.startIndex + PrimitiveIndex()];

    float3 v0 = globalData.vertices[idx.x];
    float3 v1 = globalData.vertices[idx.y];
    float3 v2 = globalData.vertices[idx.z];
  
    const float3 pos = v0 * b.x + v1 * b.y + v2 * b.z;
  
    float3 n0 = globalData.normals[idx.x];
    float3 n1 = globalData.normals[idx.y];
    float3 n2 = globalData.normals[idx.z];
  
    const float3 n = n0 * b.x + n1 * b.y + n2 * b.z;

    const float3 worldPos = mul(ObjectToWorld(), float4(pos, 1.0));
    const float3 worldN   = normalize(mul(n, WorldToObject()).xyz);

    prd.position = worldPos;
    prd.normal   = worldN;
    prd.emission = mat.emission.rgb * mat.emissionStrength;

    matrix<float, 3, 3> frame = CreateONB(worldN);
    float3 wo = mul(prd.wi, transpose(frame));

    Optional<BxDFSample> sample;
    switch(mat.type) {
        case 1: {
            DielectricBSDF bsdf = DielectricBSDF(mat.ior.x, CalculateGGXAlpha(mat.roughness, mat.anisotropy));
            sample = bsdf.Sample(wo, prd.s0, prd.s1);
            break;
        }
        case 2: {
            float3 f0;
            if (mat.baseColorTexture >= 0) {
                float2 tx0 = globalData.texCoords[idx.x];
                float2 tx1 = globalData.texCoords[idx.y];
                float2 tx2 = globalData.texCoords[idx.z];
            
                const float2 tx = tx0 * b.x + tx1 * b.y + tx2 * b.z;
                const float4 texSample = textures[mat.baseColorTexture].SampleLevel(tx, 0.0);

                if (texSample.a > 0.5) {
                    f0 = texSample.rgb;
                } else {
                    f0 = mat.f0.rgb;
                }
            } else {
                f0 = mat.f0.rgb;
            }
            ConductorBRDF brdf = ConductorBRDF(f0, mat.roughness, mat.anisotropy, textures[GGX_REFLECTION_COMPENSATION_LUT_INDEX]);
            sample = brdf.Sample(wo, prd.s0, prd.s1);
            break;
        }
        case 3: {
            ThinDielectricBSDF bsdf = ThinDielectricBSDF(mat.ior.x, mat.transmissionColor.rgb);
            sample = bsdf.Sample(wo, prd.s0, prd.s1);
            break;
        }
        case 4: {
            float3 diffuse;
            if (mat.baseColorTexture >= 0) {
                float2 tx0 = globalData.texCoords[idx.x];
                float2 tx1 = globalData.texCoords[idx.y];
                float2 tx2 = globalData.texCoords[idx.z];
            
                const float2 tx = tx0 * b.x + tx1 * b.y + tx2 * b.z;
                const float4 texSample = textures[mat.baseColorTexture].SampleLevel(tx, 0.0);
                if (texSample.a > 0.5) {
                    diffuse = texSample.rgb;
                } else {
                    diffuse = mat.f0.rgb;
                }
            } else {
                diffuse = mat.diffuse.rgb;
            }

            float3 Rs = lerp(0, 0.08, mat.specularTint);
            GlossyDiffuseBRDF brdf = GlossyDiffuseBRDF(diffuse, Rs, CalculateGGXAlpha(mat.roughness, mat.anisotropy));
            sample = brdf.Sample(wo, prd.s0, prd.s1);
            break;
        }
        case 5: {
            float3 diffuse;
            if (mat.baseColorTexture >= 0) {
                float2 tx0 = globalData.texCoords[idx.x];
                float2 tx1 = globalData.texCoords[idx.y];
                float2 tx2 = globalData.texCoords[idx.z];
            
                const float2 tx = tx0 * b.x + tx1 * b.y + tx2 * b.z;
                const float4 texSample = textures[mat.baseColorTexture].SampleLevel(tx, 0.0);
                if (texSample.a > 0.5) {
                    diffuse = texSample.rgb;
                } else {
                    diffuse = mat.f0.rgb;
                }
            } else {
                diffuse = mat.diffuse.rgb;
            }

            OrenNayarBRDF brdf = OrenNayarBRDF(diffuse, mat.diffuseRoughness);
            sample = brdf.Sample(wo, prd.s0, prd.s1);
            break;
        }
        default: {
            float3 diffuse;
            if (mat.baseColorTexture >= 0) {
                float2 tx0 = globalData.texCoords[idx.x];
                float2 tx1 = globalData.texCoords[idx.y];
                float2 tx2 = globalData.texCoords[idx.z];
            
                const float2 tx = tx0 * b.x + tx1 * b.y + tx2 * b.z;
                const float4 texSample = textures[mat.baseColorTexture].SampleLevel(tx, 0.0);
                if (texSample.a > 0.5) {
                    diffuse = texSample.rgb;
                } else {
                    diffuse = mat.f0.rgb;
                }
            } else {
                diffuse = mat.diffuse.rgb;
            }

            DiffuseBRDF brdf = DiffuseBRDF(diffuse);
            sample = brdf.Sample(wo, prd.s0, prd.s1);
        }
    }

    if (sample == none) {
        prd.pdf = 0;
    } else {
        prd.wi  = mul(sample.value.wi, frame);
        prd.f   = sample.value.f;
        prd.pdf = sample.value.pdf;
    }
}

[shader("miss")]
void missMain(inout Payload prd) {
    // TODO: sample envmap
    prd.bMiss = true;
}