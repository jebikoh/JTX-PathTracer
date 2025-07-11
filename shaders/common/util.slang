module util;

public matrix<float, 3, 3> CreateONB(float3 n) {
    float sign = (n.z >= 0.0) ? 1.0 : -1.0;
    float a    = -1.0 / (sign + n.z);
    float b    = n.x * n.y * a;

    return {
        float3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x),
        float3(b, sign + n.y * n.y * a, -n.y),
        n
    };
}

public float3 Schlick(float3 wo, float3 n, float3 f0) {
    const float cosTheta = abs(dot(wo, n));
    const float m = 1 - cosTheta;
    const float m2 = m * m;
    return f0 + (float3(1, 1, 1) - f0) * m2 * m2 * m;
}