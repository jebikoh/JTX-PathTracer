#pragma once

#include "glad/glad.h"
#include "image.hpp"

#include <SDL_video.h>

class Display {
public:
    Display(int width, int height);

    bool init();

    void updateTexture(const RGBImage &img) const;

    void processEvents(bool &isRunning);

    void render() const;
private:
    int width_, height_;

    SDL_Window *window_;
    SDL_GLContext glContext_;

    GLuint textureId_;
    GLuint shaderProgram_;
    GLuint vao_, vbo_, ebo_;

    bool initWindow();
    bool initShaders();
    void initQuad();
    void initUI() const;
};
