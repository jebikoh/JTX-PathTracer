#define PI          3.14159265358979323
#define TAU         6.28318530717958647
#define INV_PI      0.31830988618379067
#define INV_2_PI    0.15915494309189533
#define INV_4_PI    0.07957747154594766
#define PI_OVER_4   0.78539816339744830
#define PI_OVER_2   1.57079632679489661

uint pcgState;

// https://www.jcgt.org/published/0009/03/02/
uint xxhash32(uvec3 p) {
    uint h32 = p.z + 374761393u + p.x * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      += p.y * 3266489917u;
    h32      = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32      = 2246822519u * (h32 ^ (h32 >> 15));
    h32      = 3266489917u * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

uint xxhash32(uvec4 p) {
    uint h32 = p.w + 374761393u + p.x * 3266489917u;
    h32          = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 += p.y * 3266489917u;
    h32 = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 += p.z * 3266489917u;
    h32 = 668265263u * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = 2246822519u * (h32 ^ (h32 >> 15));
    h32 = 3266489917u * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

// RXS-M-XS: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint PCG() {
    uint state = pcgState;
    pcgState   = pcgState * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void InitRNG(uint x, uint y, uint z) {
    pcgState = xxhash32(uvec3(x, y, z));
}

void InitRNG(uint seed, uint x, uint y, uint z) {
    pcgState = xxhash32(uvec4(seed, x, y, z));
}

float UniformFloat() {
    return (PCG() & 0xFFFFFF) / 16777216.0f;
}

float UniformFloat(float min, float max) {
    return min + (max - min) * UniformFloat();
}

vec2 UniformVec2() {
    return vec2(UniformFloat(), UniformFloat());
}

vec2 UniformVec2(float min, float max) {
    return vec2(UniformFloat(min, max), UniformFloat(min, max));
}

vec3 UniformVec3() {
    return vec3(UniformFloat(), UniformFloat(), UniformFloat());
}

vec3 UniformVec3(float min, float max) {
    return vec3(UniformFloat(min, max), UniformFloat(min, max), UniformFloat(min, max));
}
