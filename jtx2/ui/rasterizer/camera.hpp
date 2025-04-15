#pragma once

#include <SDL_events.h>
#include "jtx.hpp"

struct Camera {
    vec3 velocity;
    vec3 position;

    float pitch = 0.0f;
    float yaw = 0.0f;
    float speed = 1.0f;

    [[nodiscard]] mat4 getViewMatrix() const;
    [[nodiscard]] mat4 getRotationMatrix() const;

    void processSDLEvent(SDL_Event &event);
    void update(float deltaTime = 0.0f);
    vec3 getFront() const;
};