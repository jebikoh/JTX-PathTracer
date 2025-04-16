#pragma once

#include <SDL_events.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

/**
 * Class for FPS camera
 *
 * This is temporary just to test the rasterization backend
 */
struct Camera {
    glm::vec3 velocity{};
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.f);

    float pitch = 0.0f;
    float yaw = 0.0f;
    float speed = 1.0f;

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::mat4 getRotationMatrix() const;

    void processSDLEvent(const SDL_Event &event);
    void update(float deltaTime = 0.0f);
    glm::vec3 getFront() const;
};