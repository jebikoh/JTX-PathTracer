#pragma once

#include "camera.hpp"
#include "glad/glad.h"
#include "rt.hpp"
#include "scene.hpp"

#include <SDL_video.h>

// Both in logical pixels
constexpr int SIDEBAR_WIDTH = 400;
constexpr int FONT_SIZE     = 14;

struct MouseState {
    bool leftButtonDown = false;
    bool middleButtonDown = false;
    bool rightButtonDown = false;
    bool shiftDown = false;
    int x = 0;
    int y = 0;
    int deltaX = 0;
    int deltaY = 0;
    int scroll = 0;
    bool isOverViewport = false;
};

class Display {
public:
    Display(int width, int height, Camera *camera);

    bool init();

    void destroy() const;

    void processEvents(bool &isRunning);

    void render();

    void setScene(Scene *scene) {
        scene_ = scene;
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

    Scene *scene_;
    bool rebuildBVH_ = false;

    SDL_Window *window_;
    SDL_GLContext glContext_;

    GLuint textureId_;
    GLuint shaderProgram_;
    GLuint vao_, vbo_, ebo_;

    MouseState mState_;
    bool interactiveMode_ = false;
    float camSensitivity_ = false;
    bool resetRender_ = false;

    void panCamera(int deltaX, int deltaY);
    void zoomCamera(int scroll);
    void rotateCamera();

    bool initWindow();
    bool initShaders();
    void initQuad();
    void initUI() const;

    void renderScene();
    void renderMenuBar(bool inputDisabled);
    void renderConfig();
    void renderSceneEditor();
    bool isRendering_ = false;

    void updateScale();
};
