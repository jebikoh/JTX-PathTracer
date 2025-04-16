#include "camera.hpp"
#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

glm::mat4 Camera::getViewMatrix() const {
    const glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 cameraRotation    = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() const {
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    glm::quat yawRotation   = glm::angleAxis(yaw, glm::vec3(0, -1, 0));
    return glm::toMat4(glm::quat(yawRotation) * glm::quat(pitchRotation));
}

void Camera::processSDLEvent(const SDL_Event &event) {
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

void Camera::update(const float deltaTime) {
    const glm::mat4 rot = getRotationMatrix();
    position += glm::vec3(rot * glm::vec4(velocity * speed * deltaTime, 0.0f));
}

glm::vec3 Camera::getFront() const {
    const glm::mat4 rot = getRotationMatrix();
    return -glm::vec3(rot[2]);
}