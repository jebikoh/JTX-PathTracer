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