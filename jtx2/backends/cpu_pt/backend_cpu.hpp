#pragma once
#include <scene/scene.hpp>
#include <backends/cpu_pt/camera.hpp>
#include <backends/cpu_pt/bvh.hpp>

namespace jtx {

class BackendCPU {
public:
    void init(const uint32_t width, const uint32_t height, const RenderSettings &settings, const CameraSettings &cameraSettings) {
        m_width = width;
        m_height = height;
        m_renderSettings = settings;

        // The camera keeps a copy of these settings
        m_camera.width = width;
        m_camera.height = height;
        m_camera.sppRow = settings.sppRow;
        m_camera.sppCol = settings.sppCol;
        m_camera.settings = cameraSettings;

        m_accBuffer.resize(width, height, 3);
        m_imgBuffer.resize(width, height, 3);
    }

    void destroy() {
        m_bvh.destroy();
        m_accBuffer.destroy();
        m_imgBuffer.destroy();
    }

    void setScene(Scene *scene) {
        m_scene = scene;
        m_bvh.build(*m_scene);
    }

    void startRendering();

    void updateRenderSettings(const RenderSettings &settings) {
        m_renderSettings = settings;
        m_renderSettings.sppRow = settings.sppRow;
        m_renderSettings.sppCol = settings.sppCol;
    }

    void updateCameraSettings(const CameraSettings &settings) {
        m_camera.settings = settings;
    }

    void resize(const int width, const int height) {
        m_width = width;
        m_height = height;

        m_camera.width = width;
        m_camera.height = height;
    }
private:
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    RenderSettings m_renderSettings{};

    Scene *m_scene = nullptr;
#ifdef JTX_USE_EMBREE
    BVHEmbree m_bvh;
#else
    BVH2 m_bvh;
#endif

    ThinLensCamera m_camera{};

    Image32f m_accBuffer;
    Image8u m_imgBuffer;
};

}