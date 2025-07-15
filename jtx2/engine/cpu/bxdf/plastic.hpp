#pragma once

namespace jtx {

inline float DiffuseRe(const float eta) {
    const float invEta = 1.0f / eta;
    const float invEta2 = invEta * invEta;
    const float invEta3 = invEta2 * invEta;
    const float invEta4 = invEta3 * invEta;
    const float invEta5 = invEta4 * invEta;

    return 0.919317 - 3.4793 * invEta + 6.75335 * invEta2 - 7.80989 * invEta3 + 4.98554 * invEta4 - 1.36881 * invEta5;
}

class PlasticBxDF {
public:
    explicit PlasticBxDF(const vec3 &reflectance, const float eta)
        : m_R(reflectance),
          m_eta(eta) {
        m_ReInt = DiffuseRe(m_eta);
    }

    vec3 Evaluate(const vec3 &wo, const vec3 &wi) const {
        return {};
    }

    bool Sample(const vec3 &wo, float s0, const vec2 &s1, BxDFSample &s) const {
        const float R = Fresnel(CosTheta(wo), m_eta);
        const float T = 1.0f - R;

        if (s0 < R) {
            // Reflection
            s.wi  = vec3(-wo.x, -wo.y, wo.z);
            s.f   = vec3(R / AbsCosTheta(s.wi));
            s.pdf = R;
        } else {
            // Transmission/scattering
            s.eta                 = m_eta;
            const bool bRefracted = Refract(wo, vec3(0.0f, 0.0f, 1.0f), s.eta, s.wi);
            if (!bRefracted) return false;
            s.f             = vec3(T) / AbsCosTheta(s.wi) / (s.eta * s.eta); // Non-symmetry
            s.pdf           = T;
            s.bTransmission = true;
        }
        s.bSpecular = true;

        return true;
    }

    float PDF(const vec3 &wo, const vec3 &wi) const {
        return {};
    }

private:
    vec3 m_R;

    float m_eta;

    float m_ReInt;
};

}// namespace jtx