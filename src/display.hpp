#pragma once

#include "camera.hpp"
#include "glad/glad.h"

#include <SDL_video.h>

constexpr int SIDEBAR_WIDTH = 300;

class Display {
public:
    Display(int width, int height, Camera *camera);

    bool init();

    void destroy() const;

    void processEvents(bool &isRunning);

    void render();

    void setWorld(HittableList *world) { this->world = world; }
private:
    int width_, height_;
    Camera *camera_;

    int renderWidth_;
    float scaleX_, scaleY_;

    SDL_Window *window_;
    SDL_GLContext glContext_;

    GLuint textureId_;
    GLuint shaderProgram_;
    GLuint vao_, vbo_, ebo_;

    HittableList *world;

    bool initWindow();
    bool initShaders();
    void initQuad();
    void initUI() const;

    void renderWorld();
    bool isRendering_ = false;

    void updateScale();
    int getWindowScale() const;
};
