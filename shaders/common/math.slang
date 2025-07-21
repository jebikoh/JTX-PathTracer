module math;

[ForceInline]
public float Pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

[ForceInline]
public float CosTheta(float3 w) { return w.z; }

[ForceInline]
public float Cos2Theta(float3 w) { return w.z * w.z; }

[ForceInline]
public float AbsCosTheta(float3 w) { return abs(w.z); }

[ForceInline]
public float Sin2Theta(float3 w) { return max(0.0f, 1.0f - Cos2Theta(w)); }

[ForceInline]
public float SinTheta(float3 w) { return sqrt(Sin2Theta(w)); }

[ForceInline]
public float TanTheta(float3 w) { return SinTheta(w) / CosTheta(w); }

[ForceInline]
public float Tan2Theta(float3 w) {
    const float cos2Theta = Cos2Theta(w);
    const float sin2Theta = max(0.0f, 1.0f - cos2Theta);
    return sin2Theta / cos2Theta;
}

[ForceInline]
public float Tan2Theta(float3 w, const float cos2Theta) {
    const float sin2Theta = max(0.0f, 1.0f - cos2Theta);
    return sin2Theta / cos2Theta;
}

[ForceInline]
public float CosPhi(float3 w) {
    const float s = SinTheta(w);
    return (s == 0) ? 1 : clamp(w.x / s, -1.0f, 1.0f);
}

[ForceInline]
public float SinPhi(float3 w) {
    const float s = SinTheta(w);
    return (s == 0) ? 0 : clamp(w.y / s, -1.0f, 1.0f);
}

[ForceInline]
public float SafeSqrt(float x) {
    return sqrt(max(0, x));
}

[ForceInline]
public float Sqr(float x) {
    return x * x;
}

[ForceInline]
public float AbsDot(float3 a, float3 b) {
    return abs(dot(a, b));
}

[ForceInline]
public bool SameHemisphere(float3 w, float3 wp) {
    return w.z * wp.z > 0;
}

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

public float Fresnel(float cosTheta, float eta) {
    cosTheta = clamp(cosTheta, -1.0, 1.0);
    if (cosTheta < 0) {
        eta = 1 / eta;
        cosTheta = -cosTheta;
    }

    const float r = max(0.0f, 1 - cosTheta * cosTheta) / (eta * eta);
    if (r >= 1) return 1.0f; // Total internal reflection
    const float cosThetaT = sqrt(max(0.0, 1.0 - r));

    const float rParallel = (eta * cosTheta - cosThetaT) / (eta * cosTheta + cosThetaT);
    const float rPerpendicular = (cosTheta - eta * cosThetaT) / (cosTheta + eta * cosThetaT);

    return (rParallel * rParallel + rPerpendicular * rPerpendicular) / 2.0f;
}

public bool Refract(float3 wi, float3 n, inout float eta, inout float3 wt) {
    float cosThetaI = dot(wi, n);

    // Ray is exiting, flip parameters
    if (cosThetaI < 0.0f) {
        eta = 1 / eta;
        cosThetaI = -cosThetaI;
        n = -n;
    }

    // Compute Snell's law
    const float r = max(0.0f, 1 - (cosThetaI * cosThetaI)) / (eta * eta);
    if (r >= 1) return false; // TIR
    const float cosThetaT = SafeSqrt(1 - r);

    wt = -wi / eta + (cosThetaI / eta - cosThetaT) * n;
    return true;
}

[ForceInline]
public float3 Reflect(float3 wo, float3 n) {
    return -wo + 2.0f * dot(wo, n) * n;
}
