module bxdf;

import sampling;
import constants;
import math;

static const float GGX_THRESHOLD    = 1e-3;

public float2 CalculateGGXAlpha(float roughness, float anisotropy) {
    float a  = 0.9 * anisotropy;
    float r2 = roughness * roughness;
    return float2(r2 * (1 + a), r2 * (1 - a));
}

struct GGX {
    private float2 m_alpha;

    __init() {
        m_alpha = float2(0, 0);
    }

    __init(float2 alpha) {
        m_alpha = alpha;
    }

    bool IsSmooth() {
        return max(m_alpha.x, m_alpha.y) < GGX_THRESHOLD;
    }

    float EvaluateNDF(const float3 wm) {
        const float cos2Theta = Cos2Theta(wm);
        const float tan2Theta = Tan2Theta(wm, cos2Theta);

        // We have to disable fp:fast and --fast-math to use this
        // TODO: explore optimization from toggling fast math for this function
        if (isinf(tan2Theta)) return 0.0f;

        const float cos4Theta = cos2Theta * cos2Theta;

        const float a = PI * m_alpha.x * m_alpha.y * cos4Theta;
        const float b = 1.0f + tan2Theta * (Sqr(CosPhi(wm) / m_alpha.x) + Sqr(SinPhi(wm) / m_alpha.y));

        return 1 / (a * b * b);
    }

    internal float Lambda(float3 w) {
        const float tan2Theta = Tan2Theta(w);
        if (isinf(tan2Theta)) return 0.0f;

        const float alpha2 = Sqr(m_alpha.x * CosPhi(w)) + Sqr(m_alpha.y * SinPhi(w));
        return 0.5 * (sqrt(1 + alpha2 * tan2Theta) - 1.0f);
    }

    float EvaluateMasking(float3 w) {
        return 1 / (1 + Lambda(w));
    }

    float EvaluateShadowingMasking(float3 wo, float3 wi) {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }

    float EvaluateVNDF(float3 w, float3 wm) {
        return EvaluateMasking(w) / AbsCosTheta(w) * EvaluateNDF(wm) * AbsDot(w, wm);
    }

    float PDF(float3 w, float3 wm) {
        return EvaluateVNDF(w, wm);
    }

    float3 SampleWm(float3 w, float2 s) {
        float3 vh = normalize(float3(m_alpha, 1.0f) * w);
        if (vh.z < 0.0f) vh = -vh;

        const float lengthSqr = vh.x * vh.x + vh.y * vh.y;
        const float3 t1 = lengthSqr > 0 ? float3(-vh.y, vh.x, 0) * (1.0f / sqrt(lengthSqr)) : float3(1, 0, 0);
        const float3 t2         = cross(vh, t1);

        float2 p = UniformDiscPolar(s);

        const float h = sqrt(1 - p.x * p.x);
        const float scale = 0.5 * (1 + vh.z);
        p.y = (1.0f - scale) * h + scale * p.y;

        const float3 nh = p.x * t1 + p.y * t2 + sqrt(max(0.0f, 1.0f - p.x * p.x - p.y * p.y)) * vh;

        return normalize(float3(m_alpha.x * nh.x, m_alpha.y * nh.y, max(1e-6f, nh.z)));
    }

}

public struct BxDFSample {
    public float3 f;
    public float3 wi;
    public float pdf;
}

interface IBxDF {
    Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1);
    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float3 wi);
}

public struct DiffuseBRDF : IBxDF {
    private float3 m_R;

    public __init(float3 reflectance) {
        m_R = reflectance;
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample out;
        out.wi = CosineHemisphere(s1);
        if (wo.z < 0) out.wi.z *= -1;
        out.pdf = CosineHemispherePDF(abs(out.wi.z));
        out.f = m_R * INV_PI;
        return out;
    }

    public float3 Evaluate(float3 wo, float3 wi) {
        return float3(0, 0, 0);
    }

    public float PDF(float3 wo, float3 wi) {
        return 0;
    }
}

public struct ConductorBRDF : IBxDF {
    private float3 m_f0;
    private GGX m_ggx;

    public __init(float3 f0) {
        m_f0 = f0;
        m_ggx = GGX();
    }

    public __init(float3 f0, float2 alpha) {
        alpha.x = max(alpha.x, GGX_THRESHOLD);
        alpha.y = max(alpha.y, GGX_THRESHOLD);

        m_f0  = f0;
        m_ggx = GGX(alpha);
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample out;
        if (m_ggx.IsSmooth()) {
            out.wi = float3(-wo.x, -wo.y, wo.z);
            const float absCosTheta = abs(out.wi.z);
            out.f = Schlick(wo, float3(0, 0, 1), m_f0) / absCosTheta;
            out.pdf = 1.0f;

            return out;
        }
        
        if (wo.z == 0.0f) return none;

        const float3 wm = m_ggx.SampleWm(wo, s1);
        out.wi = Reflect(wo, wm);
        if (!SameHemisphere(wo, out.wi)) return none;

        out.pdf = m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));

        const float absCosThetaO = AbsCosTheta(wo);
        const float absCosThetaI = AbsCosTheta(out.wi);
        if (absCosThetaO == 0 || absCosThetaI == 0) return none;

        const float D = m_ggx.EvaluateNDF(wm);
        const float3 F = Schlick(wo, wm,  m_f0);
        const float G = m_ggx.EvaluateShadowingMasking(wo, out.wi);

        out.f = D * F * G / (4 * absCosThetaI * absCosThetaO);

        return out;
    }

    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float3 wi);
}

public struct DielectricBSDF : IBxDF {
    private float m_eta;
    private GGX m_ggx;

    public __init(float eta) {
        m_eta = eta;
        m_ggx = GGX();
    }

    public __init(float eta, float2 alpha) {
        m_eta = eta;
        m_ggx = GGX(alpha);
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample out;

        if (m_eta == 1 || m_ggx.IsSmooth()) {
            const float R = Fresnel(CosTheta(wo), m_eta);
            const float T = 1.0f - R;

            if (s0 < R) {
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

        const float3 wm = m_ggx.SampleWm(wo, s1);

        const float R = Fresnel(dot(wo, wm), m_eta);
        const float T = 1 - R;

        if (s0 < R) {
            // Reflection (BRDF)
            out.wi = Reflect(wo, wm);
            if (!SameHemisphere(wo, out.wi)) return none;

            const float cosThetaO = AbsCosTheta(wo);
            const float cosThetaI = AbsCosTheta(out.wi);
            if (cosThetaO == 0 || cosThetaI == 0) return none;

            out.pdf = m_ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm)) * R;

            const float D = m_ggx.EvaluateNDF(wm);
            const float G = m_ggx.EvaluateShadowingMasking(wo, out.wi);
            out.f = float3(D * G * R / (4 * cosThetaO * cosThetaI));
        } else {
            // Transmission (BTDF)
            float eta = m_eta;
            const bool bRefracted = Refract(wo, wm, eta, out.wi);
            if (SameHemisphere(wo, out.wi) || out.wi.z == 0 || !bRefracted) return none;

            float denominator = Sqr(dot(out.wi, wm) + dot(wo, wm) / eta);
            float numerator = m_ggx.PDF(wo, wm) * AbsDot(out.wi, wm);
            out.pdf =  numerator / denominator * T;

            const float D = m_ggx.EvaluateNDF(wm);
            const float G = m_ggx.EvaluateShadowingMasking(wo, out.wi);

            numerator = D * T * G * abs(dot(out.wi, wm) * dot(wo, wm));
            denominator *= abs(CosTheta(out.wi) * CosTheta(wo));

            out.f = float3(numerator / denominator);
        }

        return out;
    }

    float3 Evaluate(float3 wo, float3 wi);
    float PDF(float3 wo, float3 wi);
}

public struct ThinDielectricBSDF : IBxDF {
    private float m_eta;

    public __init(float eta) {
        m_eta = eta;
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample out;

        float R = Fresnel(CosTheta(wo), m_eta);
        float T = 1.0f - R;

        if (R < 1) {
            R += (T * T * R) / (1 - (R * R));
            T = 1 - R;
        }

        if (s0 < R) {
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
    float PDF(float3 wo, float3 wi);
}

public struct GlossyDiffuseBRDF : IBxDF {
    private float3 m_Rd;
    private float3 m_Rs;
    private GGX m_ggx;

    public __init(float3 Rd, float3 Rs, float2 alpha) {
        m_Rd = Rd;
        m_Rs = Rs;

        alpha.x = max(GGX_THRESHOLD, alpha.x);
        alpha.y = max(GGX_THRESHOLD, alpha.y);

        m_ggx = GGX(alpha);
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample s;
        // One-sample MIS
        if (s0 < 0.5) {
            // Sample wi via diffuse lobe
            s.wi = CosineHemisphere(s1);
            if (wo.z < 0) s.wi.z *= -1;
        } else {
            // Sample wi via specular lobe
            const float3 wm = m_ggx.SampleWm(wo, s1);
            s.wi = Reflect(wo, wm);
            if (!SameHemisphere(wo, s.wi)) return none;
        }
        s.f = Evaluate(wo, s.wi);
        s.pdf = PDF(wo, s.wi);

        return s;
    }

    float PDF(float3 wo, float3 wi) {
        if (!SameHemisphere(wo, wi)) return 0.0f;
        const float3 wm = normalize(wo + wi);

        const float ps = m_ggx.PDF(wo, wm) / (4 * dot(wo, wm));
        const float pd = CosineHemispherePDF(AbsCosTheta(wi));
        return 0.5 * (ps + pd);
    }

    float3 Evaluate(float3 wo, float3 wi) {
        float3 wm = wo + wi;
        if (wm.x == 0 && wm.y == 0 && wm.z == 0) return float3(0, 0, 0);
        wm = normalize(wm);

        float3 fd = EvaluateDiffuse(wo, wi);
        float3 fs = EvaluateSpecular(wo, wi, wm);

        return fd + fs;
    }

    float3 EvaluateSpecular(float3 wo, float3 wi, float3 wm) {
        const float D = m_ggx.EvaluateNDF(wm);
        const float3  F = Schlick(wo, wm, m_Rs);
        return (D * F) / (4 * dot(wo, wm) * max(AbsCosTheta(wi), AbsCosTheta(wo)));
    }

    float3 EvaluateDiffuse(float3 wo, float3 wi) {
        const float p1 = Pow5(1 - (0.5 * AbsCosTheta(wi)));
        const float p2 = Pow5(1 - (0.5 * AbsCosTheta(wo)));
        return 28 * INV_PI_23 * m_Rd * (1 - m_Rs) * (1 - p1) * (1 - p2);
    }
}
// -- Qualitative Oren-Nayar --
float3 f_QON(float3 wo, float3 wi, float3 rho, float sigma) {
    float sigma2 = sigma * sigma;
    float Aq = 1 - 0.5 * sigma2 / (sigma2 + 0.33);
    float Bq = 0.45 * sigma2 / (sigma2 + 0.09);

    float s = dot(wo, wi) - (wi.z * wo.z);
    float invTq = 0.0;
    if (s > 0.0) {
        invTq = 1 / max(max(wi.z, wo.z), 1e-6);
    }

    return rho * INV_PI * (Aq + Bq * s * invTq);
}

// -- Fujii Oren-Nayar --
static const float FON_AF_CONST = 0.5f - (2.0f / 3.0f) * INV_PI;
static const float FON_E_AVG_CONST = (2.0f / 3.0f) - (28.0f / 15.0f * INV_PI);
static const float4 FON_G_COEFFS   = float4(0.0571085289, 0.491881867, -0.332181442, 0.0714429953);

float3 f_FON(float3 wo, float3 wi, float3 rho, float r) {
    float Af = 1 / (1 + r * FON_AF_CONST);
    float Bf = r * Af;
    float s = dot(wo, wi) - (wi.z * wo.z);
    float invTf = 1.0;
    if (s > 0.0) {
        invTf = 1 / max(max(wi.z, wo.z), 1e-6);
    }
    return rho * INV_PI * (Af + Bf * s * invTf);
}

// Average directional albedo
[ForceInline]
float Eavg_FON(float Af, float Bf) {
    return Af + FON_E_AVG_CONST * Bf;
} 

// Directional albedo
[ForceInline]
float E_FON(float cosTheta, float Af, float Bf) {
    // E_f(\omega_o) = \rho(A_F + (B_f / \pi) * G_F(\omega_o))
    //               = \rho(A_f + (B_f / \pi) * \pi * \sum_{k=1}^4 g_k(1 - \cos\theta_o)^k)
    //               = \rho(A_f + B_f * \sum_{k=1}^4 g_k(1 - \cos\theta_o)^k)
    float x  = 1 - cosTheta;
    float x2 = x * x;
    // I find this much more readable then using a 2x2 matrix...
    float gOverPi = dot(FON_G_COEFFS, float4(x, x2, x * x2, x2 * x2));
    return Af + Bf * gOverPi;
}

// -- Energy-preserving Oren-Nayar --
static const float EON_EPS         = 1e-7;

float3 f_EON(float3 wo, float3 wi, float3 rho, float r) {
    // SS (same as f_FON)
    float Af = 1.0f / (1.0f + r * FON_AF_CONST);
    float Bf = r * Af;
    float s = dot(wo, wi) - (wi.z * wo.z);
    float invTf = 1.0f;
    if (s > 0.0f) {
        invTf = 1.0f / max(wi.z, wo.z);
    }
    float3 ss = rho * INV_PI * Af * (1.0f + r * s * invTf);

    // MS
    float Ewo  = E_FON(wo.z, Af, Bf);
    float Ewi  = E_FON(wi.z, Af, Bf);
    float Eavg = Eavg_FON(Af, Bf);
    float oneMinusEavg = 1.0f - Eavg;

    float3 numerator = rho * rho * Eavg * max(EON_EPS,(1.0f - Ewo)) * max(EON_EPS,(1.0f - Ewi));
    float3 denominator = (1.0f - rho * oneMinusEavg) * max(EON_EPS,oneMinusEavg);
    float3 ms = numerator / denominator * INV_PI;

    return ss + ms;
}

public struct OrenNayarBRDF : IBxDF {
    private float3 m_rho;
    private float m_r;

    public __init(float3 rho, float roughness) {
        m_rho = rho;

#ifdef OREN_NAYAR_QON
        m_r = roughness * PI_OVER_2; // [0, 1] -> [0, PI/2]
#else
        m_r = roughness;
#endif
    }

    public Optional<BxDFSample> Sample(float3 wo, float s0, float2 s1) {
        BxDFSample out;
        out.wi = CosineHemisphere(s1);
        if (wo.z < 0) out.wi.z *= -1;
        out.pdf = CosineHemispherePDF(abs(out.wi.z));
#if defined(OREN_NAYAR_QON)
        out.f = f_QON(wo, out.wi, m_rho, m_r);
#elif defined(OREN_NAYAR_FON)
        out.f = f_FON(wo, out.wi, m_rho, m_r);
#else
        out.f = f_EON(wo, out.wi, m_rho, m_r);
#endif
        return out;
    }

    public float3 Evaluate(float3 wo, float3 wi) {
        return float3(0, 0, 0);
    }

    public float PDF(float3 wo, float3 wi) {
        return 0;
    }
}