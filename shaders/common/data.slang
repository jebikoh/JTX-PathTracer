module data;

public static const uint GGX_REFLECTION_COMPENSATION_LUT_INDEX = 0;

public struct GlobalUniformData {
    public matrix<float, 4, 4> viewProj;
    public matrix<float, 4, 4> invViewProj;
    public float3 cameraPos;

    public int32_t envmapTexture;
    public float3 envmapColor;
    public float envmapIntensity;
    public uint32_t envmapType;
    public float horizontalOffset;
    public float verticalOffset;

    public float3 *vertices;
    public float3 *normals;
    public float2 *texCoords;
    public float3 *colors;
    public uint3  *indices;
}

public struct ObjectData {
    public matrix<float, 4, 4> world;
    public matrix<float, 4, 4> normal;
    public uint32_t startIndex;
    public uint32_t materialIndex;
}

public struct MaterialData {
    public float4 diffuse;
    public float4 ior;
    public float4 f0;
    public float4 emission;
    public float4 transmissionColor;
    public float emissionStrength;
    public float roughness;
    public float anisotropy;
    public float diffuseRoughness;
    public float specularTint;
    public uint32_t type;
    public int32_t baseColorTexture;
    private uint32_t padding0;
}