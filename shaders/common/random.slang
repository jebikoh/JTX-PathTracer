module random;

internal uint xxHash32(uint3 p) {
    uint h32 = p.z + 374761393u + p.x * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      += p.y * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      = 2246822519u * (h32 ^ (h32 >> 15));
    h32      = 3266489917u * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

internal uint xxHash32(uint4 p) {
    uint h32 = p.w + 374761393u + p.x * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      += p.y * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      += p.z * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      = 2246822519u * (h32 ^ (h32 >> 15));
    h32      = 3266489917u * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

public struct RNG {
    private uint state;

    public __init(uint x, uint y, uint z) {
        state = xxHash32(uint3(x, y, z));
    }

    public __init(uint x, uint y, uint z, uint w) {
        state = xxHash32(uint4(x, y, z, w));
    }

    // RXS-M-XS: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
    [mutating]
    private uint Step() {
        uint s = state;
        state = state * 747796405u + 2891336453u;
        uint word  = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
        return (word >> 22u) ^ word;
    }
    
    [mutating]
    public float UniformFloat() {
        return (Step() & 0xFFFFFF) / 16777216.0f;
    }
    
    [mutating]
    public float UniformFloat(float min, float max) {
        return min + (max - min) * UniformFloat();
    }
    
    [mutating]
    public float2 UniformFloat2() {
        return float2(UniformFloat(), UniformFloat());
    }
    
    [mutating]
    public float2 UniformFloat2(float min, float max) {
        return float2(UniformFloat(min, max), UniformFloat(min, max));
    }
    
    [mutating]
    public float3 UniformFloat3() {
        return float3(UniformFloat(), UniformFloat(), UniformFloat());
    }
    
    [mutating]
    public float3 UniformFloat3(float min, float max) {
        return float3(UniformFloat(min, max), UniformFloat(min, max), UniformFloat(min, max));
    }    
}