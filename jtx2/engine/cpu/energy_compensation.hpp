#pragma once

#include <image.hpp>
#include <engine/cpu/bxdf/bxdf.hpp>
#include <engine/cpu/bxdf/microfacet.hpp>
#include <jtx.hpp>

namespace jtx {

constexpr uint32_t GGX_COMPENSATION_NUM_SAMPLES = 16384;
constexpr uint32_t GGX_COMPENSATION_LUT_WIDTH   = 32;
constexpr uint32_t GGX_COMPENSATION_LUT_HEIGHT  = 32;

inline JtxResult GenerateGGXReflectionCompensationLUT(std::filesystem::path &outPath, uint32_t seed) {
    LOG_INFO(GENERAL, "Generating GGX reflection compensation LUT");
    // X axis: cosThetaO
    // Y axis: sqrt(alpha)
    Sampler sampler(seed);
    Image32f LUT(GGX_COMPENSATION_LUT_WIDTH, GGX_COMPENSATION_LUT_HEIGHT, 1);

    for (int row = 0; row < GGX_COMPENSATION_LUT_HEIGHT; row++) {
        for (int col = 0; col < GGX_COMPENSATION_LUT_WIDTH; col++) {
            const float cosThetaO  = (static_cast<float>(col) + 0.5f) / static_cast<float>(GGX_COMPENSATION_LUT_WIDTH);
            const float roughness = (static_cast<float>(row) + 0.5f) / static_cast<float>(GGX_COMPENSATION_LUT_HEIGHT);
            const float alpha = roughness * roughness;

            // Not sure if we actually have to vary the output angle
            // Might be useful eventually if we want to add a dimension for anisotropy
            // const float xy    = std::sqrt(1.0f - cosThetaO * cosThetaO);
            // const float theta = sampler.Uniform<float>() * TWO_PI;
            // const float x     = xy * std::cos(theta);
            // const float y     = xy * std::sin(theta);
            // const auto wo     = Normalize(vec3(x, y, cosThetaO));

            // Isotropic only?
            // https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
            const float x = std::sqrt(1.0f - cosThetaO * cosThetaO);
            const auto wo = Normalize(vec3(x, 0.0f, cosThetaO));

            GGX ggx{vec2(alpha, alpha)};

            float accumulation = 0.0f;

            // E_ss(\omega_o) = \int_{H^2}f(\omega_o, \omega_i)|\omega_i n|d\omega_i
            for (int sample = 0; sample < GGX_COMPENSATION_NUM_SAMPLES; sample++) {
                // This is equivalent to ConductorBxDF.Evaluate() without smooth case
                const vec2 s = sampler.Uniform<vec2>();

                const vec3 wm         = ggx.SampleWm(wo, s);
                const vec3 wi         = Reflect(wo, wm);
                if (!SameHemisphere(wo, wi)) continue;

                const float cosThetaI = AbsCosTheta(wi);
                if (cosThetaI == 0.0f) continue;

                const float D = ggx.EvaluateNDF(wm);
                const float F = 1.0f;
                const float G = ggx.EvaluateShadowingMasking(wo, wi);

                const float f   = D * F * G / (4 * cosThetaO * cosThetaI);
                const float pdf = ggx.PDF(wo, wm) / (4 * AbsDot(wo, wm));
                if (pdf > 0.0f) accumulation += f * cosThetaI / pdf;
            }

            const float ess = accumulation / GGX_COMPENSATION_NUM_SAMPLES;
            *JTX_IMAGE_PIXEL_PTR(LUT, row, col) = ess;
        }
    }

    LOG_INFO(GENERAL, "Saving LUT");
    const auto res = LUT.SaveLUT(outPath);
    LUT.Destroy();
    return res;
}

}// namespace jtx