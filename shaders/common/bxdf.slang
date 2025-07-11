module bxdf;

import sampling;
import constants;

public struct BxDFSample {
    public float3 f;
    public float3 wi;
    public float pdf;
}

interface IBxDF {
    BxDFSample Sample(float3 wo, float2 s);
    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float wi);
}

public struct DiffuseBRDF : IBxDF {
    private float3 R;

    public __init(float3 reflectance) {
        R = reflectance;
    }

    public BxDFSample Sample(float3 wo, float2 s) {
        BxDFSample out;
        out.wi = CosineHemisphere(s);
        if (wo.z < 0) out.wi.z = -1;
        out.pdf = CosineHemispherePDF(abs(out.wi.z));
        out.f = R * INV_PI;
        return out;
    }

    public float3 Evaluate(float3 wo, float3 wi) {
        return float3(0, 0, 0);
    }

    public float PDF(float3 wo, float wi) {
        return 0;
    }
}
