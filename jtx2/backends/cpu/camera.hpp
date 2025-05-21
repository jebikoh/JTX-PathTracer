#pragma once
#include <cstdint>
#include <jtx.hpp>

namespace jtx {

// TODO: move definition of RenderSettings and CameraSettings to general backend scope
struct RenderSettings {
    int32_t width  = 1920;
    int32_t height = 1080;
    int32_t sppRow = 16;
    int32_t sppCol = 16;
    int32_t maxDepth = 32;
};

// TODO: convert orientation to quaternion
struct CameraSettings {
    vec3 position;
    vec3 target;
    vec3 up;

    float yfov;
    float focalLength;
    float aperture;
};

class StaticCamera {
public:
    
private:
    RenderSettings settings;
    float m_aspectRatio = 1920.0f / 1080.0f;
};

}// namespace jtx