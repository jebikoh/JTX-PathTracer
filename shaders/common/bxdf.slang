module bxdf;

import sampling;
import constants;
import util;

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
    private float3 m_R;

    public __init(float3 reflectance) {
        m_R = reflectance;
    }

    public BxDFSample Sample(float3 wo, float2 s) {
        BxDFSample out;
        out.wi = CosineHemisphere(s);
        if (wo.z < 0) out.wi.z = -1;
        out.pdf = CosineHemispherePDF(abs(out.wi.z));
        out.f = m_R * INV_PI;
        return out;
    }

    public float3 Evaluate(float3 wo, float3 wi) {
        return float3(0, 0, 0);
    }

    public float PDF(float3 wo, float wi) {
        return 0;
    }
}

public struct ConductorBRDF : IBxDF {
    private float3 m_f0;

    public __init(float3 f0) {
        m_f0 = f0;
    }

    public BxDFSample Sample(float3 wo, float2 s) {
        BxDFSample out;
        
        out.wi = float3(-wo.x, -wo.y, wo.z);
        const float absCosTheta = abs(out.wi.z);
        out.f = Schlick(wo, float3(0, 0, 1), m_f0) / absCosTheta;
        out.pdf = 1.0f;

        return out;
    }

    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float wi);
}