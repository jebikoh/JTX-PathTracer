module sampling;

import constants;

public float3 SampleUniformSphere(float2 s) {
    const float z = 1 - 2 * s.x;
    const float a = sqrt(1.0f - z * z);
    const float phi = TAU * s.y;
    return float3(cos(phi) * a, sin(phi) * a, z);
}

public float UniformSpherePDF() {
    return INV_4_PI;
}

public float2 UniformDiscPolar(float2 s) {
    const float r     = sqrt(s.x);
    const float theta = TAU * s.y;
    return float2(r * cos(theta), r * sin(theta));
}

public float2 UniformDiskConcentric(float2 s) {
    const float2 offset = 2.0f * s - float2(1.0f, 1.0f);
    if (offset.x == 0 && offset.y == 0) return float2(0.0f, 0.0f);

    float r, theta;
    if (abs(offset.x) > abs(offset.y)) {
        // X is dominant axis
        r = offset.x;
        theta = PI_OVER_4 * (offset.y / offset.x);
    } else {
        // Y is dominant axis
        r = offset.y;
        theta = PI_OVER_2 - PI_OVER_4 * (offset.x / offset.y);
    }

    return float2(r * cos(theta), r * sin(theta));
}

public float3 UniformHemisphere(float2 s) {
    const float sinTheta = sqrt(1 - s.x * s.x);
    const float phi      = 2 * PI * s.y;
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, s.x);
}

public float UniformHemispherePDF() {
    return INV_2_PI;
}

public float3 CosineHemisphere(float2 s) {
    const float2 disk = UniformDiskConcentric(s);
    return float3(disk.x, disk.y, sqrt(1 - disk.x * disk.x - disk.y * disk.y));
}

public float CosineHemispherePDF(float cosTheta) {
    return cosTheta * INV_PI;
}

public float3 UniformTriangle(float2 s) {
    s.x = sqrt(s.x);
    const float b0 = 1 - s.x;
    const float b1 = s.y * s.x;
    return float3(b0, b1, 1 - b0 - b1);
}
