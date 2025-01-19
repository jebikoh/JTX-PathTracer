#pragma once

#include "camera.hpp"
#include "glad/glad.h"
#include "rt.hpp"

#include <SDL_video.h>

// Both in logical pixels
constexpr int SIDEBAR_WIDTH = 300;
constexpr int FONT_SIZE     = 14;

class Display {
public:
    Display(int width, int height, Camera *camera);

    bool init();

    void destroy() const;

    void processEvents(bool &isRunning);

    void render();

    void setWorld(const BVHTree *world) {
        world_ = world;
    }

    bool isRendering() const {
        return isRendering_;
    }
private:
    int width_, height_;
    int logicalWidth_, logicalHeight_;
    float windowScale_;
    Camera *camera_;

    int renderWidth_;
    float scaleX_, scaleY_;

    const BVHTree *world_;

    SDL_Window *window_;
    SDL_GLContext glContext_;

    GLuint textureId_;
    GLuint shaderProgram_;
    GLuint vao_, vbo_, ebo_;

    bool initWindow();
    bool initShaders();
    void initQuad();
    void initUI() const;

    void renderWorld();
    bool isRendering_ = false;

    void updateScale();
};
