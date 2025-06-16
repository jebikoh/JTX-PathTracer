#pragma once
#include <bvh/bvh.hpp>
#include <engine/cpu/camera.hpp>
#include <engine/backends.hpp>
#include <scene/scene.hpp>

namespace jtx {

class BackendCPU {
public:
    void Init(const uint32_t width, const uint32_t height, const RenderSettings &settings, const CameraSettings &cameraSettings) {
        m_width = width;
        m_height = height;
        m_renderSettings = settings;

        // The camera keeps a copy of these settings
        m_camera.width = width;
        m_camera.height = height;
        m_camera.sppRow = settings.sppRow;
        m_camera.sppCol = settings.sppCol;
        m_camera.settings = cameraSettings;

        m_accBuffer.Resize(width, height, 3);
        m_imgBuffer.Resize(width, height, 3);
    }

    void Destroy() {
        m_bvh.Destroy();
        m_accBuffer.Destroy();
        m_imgBuffer.Destroy();
    }

    void LoadScene(Scene *scene) {
        m_scene = scene;
        m_bvh.Build(*m_scene);
    }

    void StartProgressiveRender();
    void StartRender();

    void UpdateRenderSettings(const RenderSettings &settings) {
        m_renderSettings = settings;
        m_renderSettings.sppRow = settings.sppRow;
        m_renderSettings.sppCol = settings.sppCol;
    }

    void UpdateCameraSettings(const CameraSettings &settings) {
        m_camera.settings = settings;
    }

    void Resize(const int width, const int height) {
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
    BVH m_bvh;

    ThinLensCamera m_camera{};

    Image32f m_accBuffer;
    Image8u m_imgBuffer;
};

}