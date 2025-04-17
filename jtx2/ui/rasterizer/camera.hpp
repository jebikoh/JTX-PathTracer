#pragma once

#include <SDL_events.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <jtxlib/math/math.hpp>
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
        static std::unordered_map<SDL_FingerID, glm::vec2> fingers;
        static glm::vec2 lastCenter{0.0f};
        static bool bShiftHeld = false;

        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            const bool bDown = e.type == SDL_KEYDOWN;
            if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT) {
                bShiftHeld = bDown;
            }
            return;
        }

        if (e.type == SDL_FINGERDOWN) {
            fingers[e.tfinger.fingerId] = {e.tfinger.x, e.tfinger.y};

            if (fingers.size() == 2) {
                auto it           = fingers.begin();
                const glm::vec2 a = it->second;
                ++it;
                const glm::vec2 b = it->second;
                lastCenter        = 0.5f * (a + b);
            }
            return;
        }

        if (e.type == SDL_FINGERUP) {
            fingers.erase(e.tfinger.fingerId);
            return;
        }

        if (e.type == SDL_FINGERMOTION && fingers.size() == 2) {
            fingers[e.tfinger.fingerId] = {e.tfinger.x, e.tfinger.y};

            auto it           = fingers.begin();
            const glm::vec2 a = it->second;
            ++it;
            const glm::vec2 b      = it->second;
            const glm::vec2 center = 0.5f * (a + b);
            const glm::vec2 delta  = center - lastCenter;
            lastCenter             = center;

            if (bShiftHeld) {
                // Pan
                target += -delta.x * panSpeed * getRightVector();
                target += -delta.y * panSpeed * up;
            } else {
                // Orbit
                yaw -= delta.x * orbitSpeed * glm::two_pi<float>();
                pitch += delta.y * orbitSpeed * glm::pi<float>();
            }
            return;
        }

        if (e.type == SDL_MULTIGESTURE) {
            if (std::abs(e.mgesture.dDist) > 0.002f) {
                const float zoomFactor = e.mgesture.dDist > 0.0f ? 1.0f / dollySpeed : dollySpeed;
                distance               = std::max(0.01f, distance * zoomFactor);
            }
        }
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
                cosPitch * cosYaw
        );

        position = target + distance * orbitOffset;
    }
private:

};