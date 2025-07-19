#pragma once

#include <SDL3/SDL_events.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

namespace jtx {

/**
 * Orbiting camera
 *
 * Controls mimic Blender
 */
class OrbitCamera {
public:
    // -- Position State --
    glm::vec3 target      = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 position    = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float distance = 10.0f;

    // -- Lens & projection --
    Camera::Settings settings = {};

    // -- Speed controls --
    float orbitSpeed = 1.0f;
    float dollySpeed = 1.1f;
    float panSpeed   = 4.0f;

    OrbitCamera() {
        SetView(glm::vec3(5.0, 5.0, 5.0), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Update();
    }

    OrbitCamera(const glm::vec3 &pos, const glm::vec3 &tgt, const glm::vec3 &up) {
        SetView(pos, tgt, up);
        Update();
    }

    void SetView(const glm::vec3& newPos, const glm::vec3& newTarget, const glm::vec3& newUp) {
        target = newTarget;
        distance = glm::length(newPos - newTarget);
        if (distance > 0.0001f) {
            orientation = glm::quatLookAt(glm::normalize(newTarget - newPos), newUp);
        }
        m_bCameraChanged = true;
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(position, target, GetUpVector());
    }

    glm::mat4 GetProjectionMatrix(const float aspectRatio, const float nearClip, const float farClip) const {
        const float sensorHeight = settings.sensorWidth / aspectRatio;
        const float fovY = 2.0f * glm::atan(sensorHeight / (2.0f * settings.focalLength));
        return glm::perspective(fovY, aspectRatio, nearClip, farClip);
    }

    glm::vec3 GetFrontVector() const { return orientation * glm::vec3(0, 0, -1); }
    glm::vec3 GetRightVector() const { return orientation * glm::vec3(1, 0, 0); }
    glm::vec3 GetUpVector() const { return orientation * glm::vec3(0, 1, 0); }

    void NotifyChanged() { m_bCameraChanged = true; }
    bool HasChanged() const { return m_bCameraChanged; }
    void Update() {
        position = target - GetFrontVector() * distance;
        m_bCameraChanged = false;
    }

    void ProcessSDLEvent(const SDL_Event &e) {
        if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
            const bool bDown = e.type == SDL_EVENT_KEY_DOWN;
            switch (e.key.key) {
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    m_bShiftHeld = bDown;
                    break;
                case SDLK_LALT:
                case SDLK_RALT:
                    m_bAltHeld = bDown;
                    break;
                default:
                    return;
            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            const bool bDown = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                m_bMmbHeld = bDown;
                if (bDown) {
                    m_lastMousePos = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
                }
            }
            return;
        }

        if (e.type == SDL_EVENT_FINGER_DOWN) {
            m_fingers[e.tfinger.fingerID] = {e.tfinger.x, e.tfinger.y};

            if (m_fingers.size() == 2) {
                auto it           = m_fingers.begin();
                const glm::vec2 a = it->second;
                ++it;
                const glm::vec2 b = it->second;
                m_lastCenter      = 0.5f * (a + b);
            }
            return;
        }

        if (e.type == SDL_EVENT_FINGER_UP) {
            m_fingers.erase(e.tfinger.fingerID);
            if (m_fingers.size() < 2) {
            }
            return;
        }

        if (e.type == SDL_EVENT_MOUSE_WHEEL && m_fingers.size() == 0) {
            if (e.wheel.y != 0) {
                const float zoomFactor = (e.wheel.y > 0) ? (1.0f / dollySpeed) : dollySpeed;
                distance               = std::max(0.01f, distance * std::powf(zoomFactor, std::abs(e.wheel.y)));
                m_bCameraChanged = true;
            }
            return;
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION && m_bMmbHeld) {
            const glm::vec2 curr{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
            const glm::vec2 deltaPixel = curr - m_lastMousePos;
            m_lastMousePos             = curr;
            const glm::vec2 delta      = deltaPixel * 0.002f;

            if (m_bShiftHeld) {
                target += -delta.x * panSpeed * GetRightVector();
                target += delta.y * panSpeed * GetUpVector();
            } else {
                const float dYaw   = -delta.x * orbitSpeed * glm::two_pi<float>();
                const float dPitch = -delta.y * orbitSpeed * glm::pi<float>();

                orientation = glm::angleAxis(dYaw, glm::vec3(0, 1, 0)) * orientation;
                glm::vec3 right = orientation * glm::vec3(1, 0, 0);
                orientation = glm::angleAxis(dPitch, right) * orientation;
            }
            m_bCameraChanged = true;
            return;
        }

        // TODO: Update trackpad code for SDL3
        // Trackpad
        // if (m_bAltHeld) {
        //     if (e.type == SDL_MULTIGESTURE && std::abs(e.mgesture.dDist) > 0.002f) {
        //         const float zoomFactor = e.mgesture.dDist > 0.0f ? 1.0f / dollySpeed : dollySpeed;
        //         distance               = std::max(0.01f, distance * zoomFactor);
        //         m_bCameraChanged = true;
        //     }
        //     return;
        // }
        //
        // if (e.type == SDL_EVENT_FINGER_MOTION  && m_fingers.size() == 2) {
        //     m_fingers[e.tfinger.fingerID] = {e.tfinger.x, e.tfinger.y};
        //
        //     auto it           = m_fingers.begin();
        //     const glm::vec2 a = it->second;
        //     ++it;
        //     const glm::vec2 b      = it->second;
        //     const glm::vec2 center = 0.5f * (a + b);
        //     const glm::vec2 delta  = center - m_lastCenter;
        //     m_lastCenter           = center;
        //
        //     if (m_bShiftHeld) {
        //         target += -delta.x * panSpeed * GetRightVector();
        //         target += delta.y * panSpeed * GetUpVector();
        //     } else {
        //         const float dYaw   = -delta.x * orbitSpeed * glm::two_pi<float>();
        //         const float dPitch = -delta.y * orbitSpeed * glm::pi<float>();
        //
        //         orientation = glm::angleAxis(dYaw, glm::vec3(0, 1, 0)) * orientation;
        //         glm::vec3 right = orientation * glm::vec3(1, 0, 0);
        //         orientation = glm::angleAxis(dPitch, right) * orientation;
        //     }
        //
        //     m_bCameraChanged = true;
        // }
    }

    /**
     * This resets input tracking state; should be called any time SDL events
     * are not forwarded to this class
     */
    void ResetInputState() {
        m_fingers.clear();
        m_lastCenter   = glm::vec2(0.0f);
        m_lastMousePos = glm::vec2(0.0f);
        m_bShiftHeld   = false;
        m_bAltHeld     = false;
        m_bMmbHeld     = false;
    }

private:
    bool m_bCameraChanged = false;
    std::unordered_map<SDL_FingerID, glm::vec2> m_fingers;
    glm::vec2 m_lastCenter{0.0f};
    glm::vec2 m_lastMousePos{0.0f};
    bool m_bShiftHeld = false;
    bool m_bAltHeld   = false;
    bool m_bMmbHeld   = false;
};

}
