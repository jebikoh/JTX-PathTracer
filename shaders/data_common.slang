module data_common;

public struct GlobalUniformData {
    public matrix<float, 4, 4> viewProj;
    public matrix<float, 4, 4> invViewProj;
    public float4 cameraPos;
    public float3 sunDirection;
    public float sunIntensity;
    
    public float3 *vertices;
    public float3 *normals;
    public float2 *texCoords;
    public float3 *colors;
    public uint32_t3 *indices;
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
    public float4 k;
    public float4 f0;
    public float4 emission;
    public float4 roughness;
    public uint32_t type;
    public int32_t diffuseTexture;
}