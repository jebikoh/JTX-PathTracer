module microfacet;
import math;
import sampling;
import constants;

static const float GGX_THRESHOLD    = 1e-3;

public struct GGX {
    private float2 m_alpha;

    public __init() {
        m_alpha = float2(0, 0);
    }

    public __init(float2 alpha) {
        m_alpha = alpha;
    }

    public bool IsSmooth() {
        return max(m_alpha.x, m_alpha.y) < GGX_THRESHOLD;
    }

    public float EvaluateNDF(const float3 wm) {
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

    public float EvaluateMasking(float3 w) {
        return 1 / (1 + Lambda(w));
    }

    public float EvaluateShadowingMasking(float3 wo, float3 wi) {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }

    public float EvaluateVNDF(float3 w, float3 wm) {
        return EvaluateMasking(w) / AbsCosTheta(w) * EvaluateNDF(wm) * AbsDot(w, wm);
    }

    public float PDF(float3 w, float3 wm) {
        return EvaluateVNDF(w, wm);
    }

    public float3 SampleWm(float3 w, float2 s) {
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
