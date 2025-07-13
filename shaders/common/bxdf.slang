module bxdf;

import sampling;
import constants;
import math;

public struct BxDFSample {
    public float3 f;
    public float3 wi;
    public float pdf;
}

interface IBxDF {
    Optional<BxDFSample> Sample(float3 wo, float2 s);
    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float wi);
}

public struct DiffuseBRDF : IBxDF {
    private float3 m_R;

    public __init(float3 reflectance) {
        m_R = reflectance;
    }

    public Optional<BxDFSample> Sample(float3 wo, float2 s) {
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

    public Optional<BxDFSample> Sample(float3 wo, float2 s) {
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

// TODO
public struct DielectricBSDF : IBxDF {
    private float m_eta;

    public __init(float eta) {
        m_eta = eta;
    }

    public Optional<BxDFSample> Sample(float3 wo, float2 s) {
        BxDFSample out;

        const float R = Fresnel(CosTheta(wo), m_eta);
        const float T = 1.0f - R;

        if (s.x < R) {
                // Reflection (BRDF)
                out.wi  = float3(-wo.x, -wo.y, wo.z);
                out.f   = float3(R / AbsCosTheta(out.wi));
                out.pdf = R;
        } else {
            // Transmission (BTDF)
            float eta = m_eta;
            const bool bRefracted = Refract(wo, float3(0.0f, 0.0f, 1.0f), eta, out.wi);
            if (!bRefracted) return none;
            out.f             = float3(T) / AbsCosTheta(out.wi) / (eta * eta); // Non-symmetry
            out.pdf           = T;
        }

        return out;
    }

    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float wi);
}