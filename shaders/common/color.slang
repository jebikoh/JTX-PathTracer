module color;

float Luminance(float3 color) {
    return 0.2125 * color.x + 0.7154 * color.y + 0.0721 * color.z;
}

float3 ApplyLuminance(float3 color, const float L1, const float L2) {
    if (L1 == 0.0f) return color;
    return color * (L2 / L1);
}

public float3 Reinhard(float3 v) {
    const float L1 = Luminance(v);
    return v / (1.0f + L1);
}