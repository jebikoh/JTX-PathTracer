#include "display.hpp"

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>

constexpr int SIDEBAR_WIDTH = 200;

const auto VERTEX_SOURCE = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;
        uniform mat4 uTransform;

        void main() {
            // Apply transformation to position
            gl_Position = uTransform * vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

const auto FRAGMENT_SOURCE = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;

        uniform sampler2D uTexture;

        void main() {
            FragColor = texture(uTexture, TexCoord);
        }
    )";

static GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compilation error: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

Display::Display(const int width, const int height, const RGBImage *img)
    : width_(width),
      height_(height),
      renderWidth_(0),
      scaleX_(0),
      scaleY_(0),
      window_(nullptr),
      glContext_(nullptr),
      textureId_(0),
      shaderProgram_(0),
      vao_(0),
      vbo_(0),
      ebo_(0),
      img_(img) {}

bool Display::init() {
    if (!initWindow()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD.\n";
        return false;
    }

    if (!initShaders()) {
        std::cerr << "Failed to initialize display shaders" << std::endl;
        return false;
    }

    initUI();

    initQuad();

    updateScale();

    return true;
}

void Display::destroy() const {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Display::setImage(const RGBImage *img) {
    this->img_ = img;
}

void Display::render() const {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img_->w_, img_->h_, 0, GL_RGB, GL_UNSIGNED_BYTE, img_->data());

    // renderwidth
    glViewport(0, 0, renderWidth_, height_);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram_);
    glBindVertexArray(vao_);

    // scaleX, scaleY
    GLint transformLoc = glGetUniformLocation(shaderProgram_, "uTransform");
    if (transformLoc != -1) {
        // Construct a scale matrix
        const float transform[16] = {
                scaleX_, 0.0f, 0.0f, 0.0f,
                0.0f, scaleY_, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uTexture"), 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glViewport(0, 0, width_, height_);

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width_ - SIDEBAR_WIDTH), 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(SIDEBAR_WIDTH), static_cast<float>(height_)), ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Sidebar", nullptr, window_flags);
    ImGui::Text("This is the sidebar area.");
    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window_);
}

bool Display::initWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window_ = SDL_CreateWindow("Ray Tracer",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               width_,
                               height_,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_MakeCurrent(window_, glContext_);
    SDL_GL_SetSwapInterval(1);
    return true;
}

bool Display::initShaders() {
    const auto vertex = compileShader(GL_VERTEX_SHADER, VERTEX_SOURCE);
    if (!vertex) { return false; }

    const auto fragment = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SOURCE);
    if (!fragment) {
        glDeleteShader(vertex);
        return false;
    }

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertex);
    glAttachShader(shaderProgram_, fragment);
    glLinkProgram(shaderProgram_);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

void Display::initQuad() {
    constexpr float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 1.0f};

    const unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0};

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Texture
    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Display::initUI() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330");
}
void Display::updateScale() {
    renderWidth_ = width_ - SIDEBAR_WIDTH;

    const auto imageWidth  = static_cast<float>(img_->w_);
    const auto imageHeight = static_cast<float>(img_->h_);

    const float scale = std::min(static_cast<float>(renderWidth_) / imageWidth, static_cast<float>(height_) / imageHeight);

    const float scaledWidth  = imageWidth * scale;
    const float scaledHeight = imageHeight * scale;

    scaleX_ = scaledWidth / static_cast<float>(renderWidth_);
    scaleY_ = scaledHeight / static_cast<float>(height_);
}

void Display::processEvents(bool &isRunning) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);

        if (e.type == SDL_QUIT) {
            isRunning = false;
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
            width_  = e.window.data1;
            height_ = e.window.data2;
            glViewport(0, 0, width_, height_);
            updateScale();
        }
    }
}