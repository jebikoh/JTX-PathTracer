#pragma once
#include <scene/scene.hpp>
#include <backends/cpu_pt/camera.hpp>
#include <backends/cpu_pt/bvh.hpp>

namespace jtx {

class CpuPtIntegrator {
public:
    void init(uint32_t width, uint32_t height);
    void destroy() const {
        m_bvh.destroy();
    }

    void setScene(Scene *scene) {
        m_scene = scene;
        m_bvh.build(*m_scene);
    }

    void startRendering() const;

    void updateRenderSettings(const RenderSettings &settings);
    void updateCameraSettings(const CameraSettings &settings);
    void resize(int width, int height);

private:
    RenderSettings m_renderSettings{};
    CameraSettings m_cameraSettings{};
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    Scene *m_scene = nullptr;
#ifdef JTX_USE_EMBREE
    BVHEmbree m_bvh;
#else
    BVH2 m_bvh;
#endif
    StaticCamera m_camera{};

    Image32f m_accBuffer;
    Image8u m_imgBuffer;
};

}