cmake_minimum_required(VERSION 3.28)

# CMAKE FLAGS
option(JTX_ENABLE_CUDA_BACKEND "Enable CUDA backend" OFF)
option(JTX_ENABLE_PROFILING "Enable profiling" ON)
option(JTX_ENABLE_PERF_FLAGS "Enable performance flags" ON)
option(JTX_ENABLE_MULTI_THREADING "Enable multi-threading" ON)
option(JTX_DISABLE_UI "Disable UI" OFF)
option(JTX_ENABLE_DEBUG_TRACES "Enable debug traces" ON)

if (JTX_ENABLE_CUDA_BACKEND)
    project(JTX VERSION 1.0.0 LANGUAGES CXX CUDA)
else ()
    project(JTX VERSION 1.0.0 LANGUAGES CXX)
endif ()

set(CMAKE_CXX_STANDARD 20)

# PROCESS CMAKE FLAGS
if (JTX_ENABLE_CUDA_BACKEND)
    include(CheckLanguage)
    check_language(CUDA)

    if (CMAKE_CUDA_COMPILER)
        set(USE_CUDA ON CACHE BOOL "Use CUDA backend" FORCE)

        add_compile_definitions(-DJTX_ENABLE_CUDA_BACKEND)

        message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "CUDA compiler version: ${CMAKE_CUDA_COMPILER_VERSION}")
    else ()
        set(USE_CUDA OFF CACHE BOOL "Use CUDA backend" FORCE)
        set(JTX_ENABLE_CUDA_BACKEND OFF CACHE BOOL "Enable CUDA backend" FORCE)
        message(STATUS "CUDA compiler not found, continuing without CUDA support")
    endif ()
else ()
    message(STATUS "CUDA support disabled")
endif ()

if (JTX_ENABLE_PROFILING)
    add_compile_definitions(-DJTX_ENABLE_PROFILING)
endif ()

if (JTX_ENABLE_MULTI_THREADING)
    add_compile_definitions(-DJTX_ENABLE_MULTI_THREADING)
endif ()

if (JTX_DISABLE_UI)
    add_compile_definitions(-DJTX_DISABLE_UI)
endif ()

if (JTX_ENABLE_DEBUG_TRACES)
    add_compile_definitions(-DJTX_ENABLE_DEBUG_TRACES)
endif ()

if (JTX_ENABLE_PERF_FLAGS)
    if (MSVC)
        message(STATUS "Using MSVC compiler")
        add_compile_options(/O2 /fp:fast)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Using GNU or Clang compiler")
        add_compile_options(-O3 -ffast-math)
    else ()
        message(WARNING "Unknown compiler. No optimization flags added.")
    endif ()
endif ()

# DEPENDENCIES
# Vulkan
find_package(Vulkan REQUIRED)
# volk
add_subdirectory(ext/volk)
# VkBootstrap
add_subdirectory(ext/vkbootstrap)
# VMA
add_subdirectory(ext/vma)
# SDL2
add_subdirectory(ext/sdl EXCLUDE_FROM_ALL)
# ImGUI
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC ext/imgui)
target_sources(imgui PRIVATE
        ext/imgui/imgui.h
        ext/imgui/imgui.cpp
        ext/imgui/imgui_demo.cpp
        ext/imgui/imgui_draw.cpp
        ext/imgui/imgui_widgets.cpp
        ext/imgui/imgui_tables.cpp
        ext/imgui/imgui_impl_vulkan.cpp
        ext/imgui/imgui_impl_sdl2.cpp
)
target_compile_definitions(imgui
        PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK
)
target_link_libraries(imgui PUBLIC volk Vulkan::Vulkan SDL2::SDL2)
# JTXlib
add_subdirectory(ext/jtxlib)
# fastglTF
add_subdirectory(ext/fastgltf)
# fmt
include(FetchContent)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

# JTX Executable
SET (JVK_HEADERS
        jtx2/ui/jvk/buffer.hpp
        jtx2/ui/jvk/commands.hpp
        jtx2/ui/jvk/context.hpp
        jtx2/ui/jvk/descriptor.hpp
        jtx2/ui/jvk/fence.hpp
        jtx2/ui/jvk/image.hpp
        jtx2/ui/jvk/init.hpp
        jtx2/ui/jvk/jvk.hpp
        jtx2/ui/jvk/pipeline.hpp
        jtx2/ui/jvk/queue.hpp
        jtx2/ui/jvk/sampler.hpp
        jtx2/ui/jvk/semaphore.hpp
        jtx2/ui/jvk/shaders.hpp
        jtx2/ui/jvk/swapchain.hpp
        jtx2/ui/jvk/util.hpp
)

set (JVK_SOURCE
        jtx2/ui/jvk/descriptor.cpp
        jtx2/ui/jvk/pipeline.cpp
        jtx2/ui/jvk/util.cpp
        jtx2/ui/jvk/immediate_buffer.hpp
        jtx2/ui/rasterizer/render_data.hpp
)

add_executable(
        JTX
        ${JVK_SOURCE}
        ${JVK_HEADERS}
        jtx2/main.cpp
        jtx2/jtx.hpp
        jtx2/util/logger.hpp
        jtx2/scene/scene.hpp
        jtx2/material.h
        jtx2/scene/scene_loader.hpp
        jtx2/image.hpp
        jtx2/ui/display.hpp
        jtx2/ui/display.cpp
        jtx2/scene/scene_loader.cpp
        jtx2/util/color.hpp
        jtx2/image.cpp
        jtx2/ui/ui_renderer.hpp
        jtx2/ui/ui_renderer.cpp
        jtx2/ui/rasterizer/rasterizer.hpp
        jtx2/ui/rasterizer/rasterizer.cpp
        jtx2/ui/rasterizer/camera.hpp
        jtx2/ui/rasterizer/camera.cpp
)


target_link_libraries(JTX PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator vk-bootstrap::vk-bootstrap volk jtxlib SDL2::SDL2main SDL2::SDL2 imgui fmt::fmt fastgltf::fastgltf)

target_include_directories(JTX
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/jtx2/
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/rapidobj
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/IconsLucide
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/imfilebrowser
        ${OPENGL_LIBRARIES}
)

add_custom_command(TARGET JTX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/jtx2/assets
        $<TARGET_FILE_DIR:JTX>/assets
        COMMENT "Copying assets directory to output folder"
)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/mesh.frag"
        "${PROJECT_SOURCE_DIR}/shaders/mesh.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV})
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)