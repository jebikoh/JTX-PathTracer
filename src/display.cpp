#include "display.hpp"

#include "bvh.hpp"
#include "camera.hpp"

#include <SDL.h>
#include <future>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>

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

void setUiTheme() {
    // Photoshop style by Derydoca from ImThemes
    ImGuiStyle &style = ImGui::GetStyle();

    style.Alpha                     = 1.0f;
    style.DisabledAlpha             = 0.6000000238418579f;
    style.WindowPadding             = ImVec2(8.0f, 8.0f);
    style.WindowRounding            = 1.0f;
    style.WindowBorderSize          = 1.0f;
    style.WindowMinSize             = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign          = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition  = ImGuiDir_Left;
    style.ChildRounding             = 4.0f;
    style.ChildBorderSize           = 1.0f;
    style.PopupRounding             = 2.0f;
    style.PopupBorderSize           = 1.0f;
    style.FramePadding              = ImVec2(4.0f, 3.0f);
    style.FrameRounding             = 2.0f;
    style.FrameBorderSize           = 1.0f;
    style.ItemSpacing               = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing          = ImVec2(4.0f, 4.0f);
    style.CellPadding               = ImVec2(4.0f, 2.0f);
    style.IndentSpacing             = 21.0f;
    style.ColumnsMinSpacing         = 6.0f;
    style.ScrollbarSize             = 13.0f;
    style.ScrollbarRounding         = 12.0f;
    style.GrabMinSize               = 7.0f;
    style.GrabRounding              = 0.0f;
    style.TabRounding               = 0.0f;
    style.TabBorderSize             = 1.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition       = ImGuiDir_Right;
    style.ButtonTextAlign           = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign       = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.2745098173618317f, 0.2745098173618317f, 0.2745098173618317f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.2980392277240753f, 0.2980392277240753f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.0f, 1.0f, 1.0f, 0.3910000026226044f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.0f, 1.0f, 1.0f, 0.6700000166893005f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.5843137502670288f, 0.5843137502670288f, 0.5843137502670288f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
}

Display::Display(const int width, const int height, Camera *camera)
    : width_(0),
      height_(0),
      logicalWidth_(width),
      logicalHeight_(height),
      camera_(camera),
      renderWidth_(0),
      scaleX_(0),
      scaleY_(0),
      window_(nullptr),
      glContext_(nullptr),
      textureId_(0),
      shaderProgram_(0),
      vao_(0),
      vbo_(0),
      ebo_(0) {}

bool Display::init() {
    if (!initWindow()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }
    updateScale();

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

void Display::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, camera_->width_, camera_->height_, 0, GL_RGB, GL_UNSIGNED_BYTE, camera_->img_.data());

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
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glViewport(0, 0, width_, height_);

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(logicalWidth_ - SIDEBAR_WIDTH), 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(SIDEBAR_WIDTH), static_cast<float>(logicalHeight_)), ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Sidebar", nullptr, window_flags);

    bool inputDisabled = false;
    if (isRendering_) {
        inputDisabled = true;

        const float progress   = static_cast<float>(camera_->currentSample_.load()) / static_cast<float>(camera_->getSpp());
        constexpr auto barSize = ImVec2(0.0f, 20.0f);
        char overlayText[64];
        snprintf(overlayText, sizeof(overlayText), "%.1f%%", progress * 100.0f);
        ImGui::ProgressBar(progress, barSize, overlayText);

        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Render")) {
        renderWorld();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        camera_->clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        camera_->save("output.png");
    }

    if (ImGui::CollapsingHeader("Configuration")) {
        // ImGui::SeparatorText("Image");
        // int dimensions[2] = {camera_->width, camera_->height};
        // ImGui::Text("Dimensions");
        // if (ImGui::InputInt2("##Dimensions", dimensions)) {
        //     camera_->resize(width_, height_);
        // }

        ImGui::SeparatorText("Ray Tracing");

        ImGui::Text("X Samples");
        ImGui::InputInt("##XSamples", &camera_->xPixelSamples_);

        ImGui::Text("Y Samples");
        ImGui::InputInt("##YSamples", &camera_->yPixelSamples_);

        ImGui::Text("Max Depth");
        ImGui::InputInt("##MaxDepth", &camera_->maxDepth_);
    }
    if (ImGui::CollapsingHeader("Camera Settings")) {
        ImGui::SeparatorText("Orientation");

        ImGui::Text("Position");
        // cast the current position to floats (stored as doubles)
        auto pos = static_cast<jtx::Vec3f>(camera_->properties_.center);
        if (ImGui::InputFloat3("##Position", &pos.x)) {
            camera_->properties_.center = static_cast<Vec3>(pos);
        }

        ImGui::Text("Target");
        auto target = static_cast<jtx::Vec3f>(camera_->properties_.target);
        if (ImGui::InputFloat3("##Target", &target.x)) {
            camera_->properties_.target = static_cast<Vec3>(target);
        }

        ImGui::Text("Up");
        auto up = static_cast<jtx::Vec3f>(camera_->properties_.up);
        if (ImGui::InputFloat3("##Up", &up.x)) {
            camera_->properties_.up = static_cast<Vec3>(up);
        }

        ImGui::SeparatorText("Lens");

        ImGui::Text("Y-FOV");
        ImGui::InputFloat("##Y-FOV", &camera_->properties_.yfov);

        ImGui::Text("Focus Angle");
        ImGui::InputFloat("##FocusAngle", &camera_->properties_.defocusAngle);

        ImGui::Text("Focus Distance");
        ImGui::InputFloat("##FocusDistance", &camera_->properties_.focusDistance);
    }

    if (ImGui::CollapsingHeader("Scene Editor")) {
        static int selectedMeshIndex = -1;
        // Scene View
        ImGui::Text("Scene View");;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Darker background
        ImGui::BeginChild("Scene View", ImVec2(0, 150), true, ImGuiWindowFlags_None);
        {
            for (size_t i = 0; i < world_->scene_.meshes.size(); ++i) {
                const auto& mesh = world_->scene_.meshes[i];
                if (ImGui::Selectable(mesh.name.c_str(), selectedMeshIndex == i)) {
                    selectedMeshIndex = i;
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (selectedMeshIndex != -1) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Darker background
            ImGui::BeginChild("Material Editor", ImVec2(0, 0), true, ImGuiWindowFlags_None);
            {
                ImGui::Text("Material Editor");
                const auto& mesh = world_->scene_.meshes[selectedMeshIndex];
                Material* material = mesh.material;

                // Material Type Combo Box
                const char* materialTypes[] = { "DIFFUSE", "DIELECTRIC", "CONDUCTOR" };
                int currentType = material->type;
                if (ImGui::Combo("Type", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
                    material->type = static_cast<Material::Type>(currentType);
                }


                switch (material->type) {
                    case Material::DIFFUSE:
                        ImGui::ColorEdit3("Albedo", &material->albedo.x);
                        break;
                    case Material::CONDUCTOR:
                        ImGui::InputFloat3("IOR", &material->IOR.x);
                        ImGui::InputFloat3("k", &material->k.x);
                        ImGui::InputFloat("Alpha X", &material->alphaX);
                        ImGui::InputFloat("Alpha Y", &material->alphaY);
                        break;
                    case Material::DIELECTRIC:
                        ImGui::InputFloat("IOR", &material->IOR.x);
                        ImGui::InputFloat("Alpha X", &material->alphaX);
                        ImGui::InputFloat("Alpha Y", &material->alphaY);
                        break;
                    default:
                        break;
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(); // Restore background color
        }
    }

    if (inputDisabled) {
        ImGui::EndDisabled();
    }
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

    window_ = SDL_CreateWindow("Otter",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               logicalWidth_,
                               logicalHeight_,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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

    SDL_GL_GetDrawableSize(window_, &width_, &height_);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Display::initUI() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("../assets/proggyvector.ttf", FONT_SIZE * windowScale_);
    io.FontGlobalScale = 1.0f / windowScale_;
    (void) io;

    setUiTheme();
    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Display::renderWorld() {
    if (isRendering_) return;

    isRendering_ = true;
    std::thread([this]() {
        camera_->render(*world_);
        isRendering_ = false;
    }).detach();
}

void Display::updateScale() {
    windowScale_ = static_cast<float>(width_) / static_cast<float>(logicalWidth_);
    renderWidth_ = width_ - (SIDEBAR_WIDTH * windowScale_);

    const auto imageWidth  = static_cast<float>(camera_->width_);
    const auto imageHeight = static_cast<float>(camera_->height_);

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
            camera_->terminateRender();
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_GetWindowSize(window_, &logicalWidth_, &logicalHeight_);
            SDL_GL_GetDrawableSize(window_, &width_, &height_);

            updateScale();
        }

        // Update mouse state
        if (e.type == SDL_MOUSEMOTION) {
            mState_.x      = e.motion.x;
            mState_.y      = e.motion.y;
            mState_.deltaX = e.motion.xrel;
            mState_.deltaY = e.motion.yrel;

            mState_.isOverViewport = (e.motion.x < (width_ - SIDEBAR_WIDTH * windowScale_));
        }

        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            bool isDown = (e.type == SDL_MOUSEBUTTONDOWN);
            if (e.button.button == SDL_BUTTON_LEFT) {
                mState_.leftButtonDown = isDown;
            } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                mState_.middleButtonDown = isDown;
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                mState_.rightButtonDown = isDown;
            }
        }

        if (e.type == SDL_MOUSEWHEEL) {
            mState_.scroll = e.wheel.y;
        }

        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool isDown = (e.type == SDL_KEYDOWN);
            if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT) {
                mState_.shiftDown = isDown;
            }
        }
    }

    if (mState_.isOverViewport) {
        if (mState_.middleButtonDown) {
            if (mState_.shiftDown) {
                // Pan the camera
            } else {
                // Rotate the camera
            }
        }
        if (mState_.scroll != 0) {
            // Zoom the camera
            mState_.scroll = 0; // Reset scroll after handling
        }
    }
}
void Display::panCamera(int deltaX, int deltaY) {
    resetRender_ = true;
    Vec3 forward = normalize(camera_->properties_.target - camera_->properties_.center);
    Vec3 right = normalize(cross(forward, camera_->properties_.up));
    Vec3 up = normalize(cross(right, forward));

    Vec3 delta = (right * static_cast<float>(-deltaX) + up * static_cast<float>(deltaY)) * camSensitivity_;
    camera_->properties_.center += delta;
    camera_->properties_.target += delta;
}
void Display::zoomCamera(int scroll) {
    resetRender_ = true;
    float zoomFactor = 1.0f + static_cast<float>(scroll) * camSensitivity_;
    camera_->properties_.center = camera_->properties_.target + (camera_->properties_.center - camera_->properties_.target) * zoomFactor;
}
void Display::rotateCamera() {
    resetRender_ = true;
}
