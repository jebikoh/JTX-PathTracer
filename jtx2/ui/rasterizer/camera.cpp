#include "camera.hpp"

mat4 Camera::getViewMatrix() const {
    const mat4 cameraTranslation = jtx::translate(position);
    const mat4 cameraRotation    = getRotationMatrix();
    return jtx::inverse(cameraTranslation * cameraRotation).value();
}

mat4 Camera::getRotationMatrix() const {
    quat pitchRotation = angleAxis(pitch, vec3(1, 0, 0));
    quat yawRotation   = angleAxis(yaw, vec3(0, -1, 0));
    return toMat4(quat(yawRotation) * quat(pitchRotation));
}

void Camera::processSDLEvent(SDL_Event &event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_w) { velocity.z = -1; }
        if (event.key.keysym.sym == SDLK_s) { velocity.z = 1; }
        if (event.key.keysym.sym == SDLK_a) { velocity.x = -1; }
        if (event.key.keysym.sym == SDLK_d) { velocity.x = 1; }
        if (event.key.keysym.sym == SDLK_SPACE) { velocity.y = 1; }
        if (event.key.keysym.sym == SDLK_LSHIFT) { velocity.y = -1; }
    }
    if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_w) { velocity.z = 0; }
        if (event.key.keysym.sym == SDLK_s) { velocity.z = 0; }
        if (event.key.keysym.sym == SDLK_a) { velocity.x = 0; }
        if (event.key.keysym.sym == SDLK_d) { velocity.x = 0; }
        if (event.key.keysym.sym == SDLK_SPACE) { velocity.y = 0; }
        if (event.key.keysym.sym == SDLK_LSHIFT) { velocity.y = 0; }
    }

    if (event.type == SDL_MOUSEMOTION) {
        yaw += static_cast<float>(event.motion.xrel) / 200.0f;
        pitch -= static_cast<float>(event.motion.yrel) / 200.0f;
    }
}

void Camera::update(float deltaTime) {
    const mat4 rot = getRotationMatrix();
    position += vec3(rot * vec4(velocity * speed * deltaTime, 0.0f));
}

vec3 Camera::getFront() const {
    const mat4 rot = getRotationMatrix();
    return -vec3(rot[2]);
}