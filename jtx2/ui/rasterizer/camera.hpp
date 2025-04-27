#pragma once

#include <SDL_events.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <jtxlib/math/math.hpp>
#include <unordered_map>
#include <util/logger.hpp>

/**
 * Class for FPS camera
 *
 * This is temporary just to test the rasterization backend
 */
struct FPSCamera {
    glm::vec3 velocity{};
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.f);

    float pitch = 0.0f;
    float yaw   = 0.0f;
    float speed = 1.0f;

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getRotationMatrix() const;

    void processSDLEvent(const SDL_Event &event);
    void update(float deltaTime = 0.0f);
    glm::vec3 getFront() const;
};

/**
 * Orbiting camera
 *
 * Controls mimic Blender
 */
class OrbitCamera {
public:
    glm::vec3 target   = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw      = 0.0f;
    float pitch    = 0.0f;
    float distance = 10.0f;

    float orbitSpeed = 1.0f;
    float dollySpeed = 1.1f;
    float panSpeed   = 1.0f;

    glm::mat4 getViewMatrix() const { return glm::lookAt(position, target, up); }
    glm::vec3 getFrontVector() const { return glm::normalize(target - position); }
    glm::vec3 getRightVector() const { return glm::normalize(glm::cross(getFrontVector(), up)); }

    void processSDLEvent(const SDL_Event &e) {
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            const bool bDown = e.type == SDL_KEYDOWN;
            switch (e.key.keysym.sym) {
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

        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            const bool bDown = e.type == SDL_MOUSEBUTTONDOWN;
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                m_bMmbHeld = bDown;
                if (bDown) {
                    m_lastMousePos = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
                }
            }
            return;
        }

        // Mouse input
        if (e.type == SDL_MOUSEWHEEL) {
            if (e.wheel.y != 0) {
                const float zoomFactor = (e.wheel.y > 0) ? (1.0f / dollySpeed) : dollySpeed;
                distance               = std::max(0.01f, distance * std::powf(zoomFactor, std::abs(e.wheel.y)));
            }
            return;
        }

        if (e.type == SDL_MOUSEMOTION && m_bMmbHeld) {
            const glm::vec2 curr{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
            const glm::vec2 deltaPixel = curr - m_lastMousePos;
            m_lastMousePos             = curr;
            const glm::vec2 delta      = deltaPixel * 0.002f;

            if (m_bShiftHeld) {
                m_gestureMode = GestureMode::Pan;
                target += -delta.x * panSpeed * getRightVector();
                target += delta.y * panSpeed * up;
            } else {
                m_gestureMode = GestureMode::Orbit;
                yaw -= delta.x * orbitSpeed * glm::two_pi<float>();
                pitch += delta.y * orbitSpeed * glm::pi<float>();
            }
            return;
        }

        // Trackpad
        if (e.type == SDL_FINGERDOWN) {
            m_fingers[e.tfinger.fingerId] = {e.tfinger.x, e.tfinger.y};

            if (m_fingers.size() == 2) {
                auto it           = m_fingers.begin();
                const glm::vec2 a = it->second;
                ++it;
                const glm::vec2 b = it->second;
                m_lastCenter      = 0.5f * (a + b);
                m_gestureMode     = GestureMode::None;
            }
            return;
        }

        if (e.type == SDL_FINGERUP) {
            m_fingers.erase(e.tfinger.fingerId);
            if (m_fingers.size() < 2) {
                m_gestureMode = GestureMode::None;
            }
            return;
        }

        if (m_bAltHeld) {
            if (e.type == SDL_MULTIGESTURE && std::abs(e.mgesture.dDist) > 0.002f) {
                const float zoomFactor = e.mgesture.dDist > 0.0f ? 1.0f / dollySpeed : dollySpeed;
                distance               = std::max(0.01f, distance * zoomFactor);
                m_gestureMode          = GestureMode::Dolly;
            }
            return;
        }

        if (e.type == SDL_FINGERMOTION && m_fingers.size() == 2) {
            m_fingers[e.tfinger.fingerId] = {e.tfinger.x, e.tfinger.y};

            auto it           = m_fingers.begin();
            const glm::vec2 a = it->second;
            ++it;
            const glm::vec2 b      = it->second;
            const glm::vec2 center = 0.5f * (a + b);
            const glm::vec2 delta  = center - m_lastCenter;
            m_lastCenter           = center;

            if (m_bShiftHeld) {
                // Pan
                m_gestureMode = GestureMode::Pan;
                target += -delta.x * panSpeed * getRightVector();
                target += -delta.y * panSpeed * up;
            } else {
                m_gestureMode = GestureMode::Orbit;
                yaw -= delta.x * orbitSpeed * glm::two_pi<float>();
                pitch += delta.y * orbitSpeed * glm::pi<float>();
            }
        }
    }

    /**
     * This resets input tracking state; should be called any time SDL events
     * are not forwarded to this class
     */
    void resetInputState() {
        m_fingers.clear();
        m_lastCenter   = glm::vec2(0.0f);
        m_lastMousePos = glm::vec2(0.0f);
        m_gestureMode  = GestureMode::None;
        m_bShiftHeld   = false;
        m_bAltHeld     = false;
        m_bMmbHeld     = false;
    }

    /**
     * This should be called once per frame after input has been handled
     */
    void update() {
        const float cosPitch = std::cos(pitch);
        const float sinPitch = std::sin(pitch);
        const float cosYaw   = std::cos(yaw);
        const float sinYaw   = std::sin(yaw);

        const glm::vec3 orbitOffset(
                cosPitch * sinYaw,
                sinPitch,
                cosPitch * cosYaw);

        position = target + distance * orbitOffset;
    }

private:
    std::unordered_map<SDL_FingerID, glm::vec2> m_fingers;
    enum class GestureMode { None,
                             Dolly,
                             Orbit,
                             Pan } m_gestureMode = GestureMode::None;
    glm::vec2 m_lastCenter{0.0f};
    glm::vec2 m_lastMousePos{0.0f};
    bool m_bShiftHeld = false;
    bool m_bAltHeld   = false;
    bool m_bMmbHeld   = false;
};
