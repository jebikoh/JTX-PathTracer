#ifdef __SLANG__

#define CPU_ONLY(x)
#define GPU_ONLY(x)         x
#define NAMESPACE_START
#define NAMESPACE_END

#define BXDF(__name)        public struct __name : IBxDF

#define BXDF_SAMPLE         public bool Sample(float3 wo, float s0, float2 s1, out BxDFSample sample)
#define BXDF_EVALUATE       public float3 Evaluate(float3 wo, float3 wi)
#define BXDF_PDF            public float PDF(float3 wo, float3 wi)
#define BXDF_FUNC_END       }

#else // C++

#define CPU_ONLY(x)         x
#define GPU_ONLY(x) 
#define NAMESPACE_START     namespace jtx
#define NAMESPACE_END       }

#define BXDF(__name)        struct __name

#define BXDF_SAMPLE         bool Sample(const float3 &wo, float s0, const float2 &s1, BxDFSample &sample) const
#define BXDF_EVALUATE       float3 Evaluate(const float3& wo, const float3& wi) const
#define BXDF_PDF            float PDF(const float3 &wo, const float3 &wi) const

#endif

GPU_ONLY(
    module bxdf;

    import microfacet;
    import sampling;
    import constants;
    import math;

    public struct BxDFSample {
        public float3 f;
        public float3 wi;
        public float pdf;
    }
    
    public interface IBxDF {
        bool Sample(float3 wo, float s0, float2 s1, out BxDFSample sample);
        float3 Evaluate(float3 wo, float3 wi);
        float PDF(float3 wo, float3 wi);
    }
)

CPU_ONLY(
    #include <jtx.hpp>
    #include <util/sampling.hpp>
    #include <engine/cpu/bxdf/bxdf.hpp>
)

NAMESPACE_START

BXDF(DiffuseBRDF) {
    float3 m_R;

    CPU_ONLY(
        explicit DiffuseBRDF(const float3 &reflectance): m_R(reflectance) {}
    )

    GPU_ONLY(
        public __init(float3 reflectance) {
            m_R = reflectance;
        }
    )

    BXDF_SAMPLE {
        sample.wi = CosineHemisphere(s1);
        if (wo.z < 0) sample.wi.z *= -1;
        sample.pdf = CosineHemispherePDF(abs(sample.wi.z));
        sample.f = m_R * INV_PI;
        return true;
    }

    BXDF_EVALUATE {
        if (!SameHemisphere(wo, wi)) return {};
        return m_R * INV_PI;
    }

    BXDF_PDF {
        if (!SameHemisphere(wo, wi)) return 0;
        return CosineHemispherePDF(AbsCosTheta(wi));
    }
}

NAMESPACE_END