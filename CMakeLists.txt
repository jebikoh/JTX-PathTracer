cmake_minimum_required(VERSION 3.28)
project(RayTracer)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_TESTS OFF CACHE BOOL "Build tests for jtxlib" FORCE)
set(NDEBUG OFF CACHE BOOL "Disable debug assertions" FORCE)

option(ENABLE_CUDA_BACKEND "Enable CUDA backend" ON)

if (ENABLE_CUDA_BACKEND)
    include(CheckLanguage)
    check_language(CUDA)

    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(USE_CUDA ON CACHE BOOL "Use CUDA backend" FORCE)

        message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "CUDA compiler version: ${CMAKE_CUDA_COMPILER_VERSION}")
    else()
        set(USE_CUDA OFF CACHE BOOL "Use CUDA backend" FORCE)
        message(STATUS "CUDA compiler not found, continuing without CUDA support")

    endif()
else()
    message(STATUS "CUDA support disabled")
endif()

option(ENABLE_PROFILING "Enable profiling" ON)
if (ENABLE_PROFILING)
    add_compile_definitions(-DENABLE_PROFILING)
endif()

option(USE_DOUBLE_PRECISION "Use double precision" OFF)
if (USE_DOUBLE_PRECISION)
    add_compile_definitions(-DUSE_DOUBLE_PRECISION)
endif()

add_compile_options(-O3 -ffast-math)

add_subdirectory(ext/jtxlib)

add_subdirectory(ext/sdl EXCLUDE_FROM_ALL)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

add_subdirectory(ext/glad)

# ImGUI
set(IMGUI_PATH  "ext/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_PATH})
target_link_libraries(imgui PRIVATE glad SDL2::SDL2)

add_executable(RayTracer src/main.cpp
        src/rt.hpp
        src/color.hpp
        src/camera.hpp
        src/image.hpp
        src/bvh.hpp
        src/material.hpp
        src/display.hpp
        src/display.cpp
        src/image.cpp
        src/scene.hpp
        src/scene.cpp
        src/camera.cpp
        src/primitives.hpp
        src/primitives.hpp
        src/util/rand.hpp
        src/util/rand.cpp
        src/util/interval.hpp
        src/util/interval.cpp
        src/util/aabb.hpp
        src/util/aabb.cpp
        src/bvh.cpp
        src/sampler.hpp
)

target_link_libraries(RayTracer PRIVATE jtxlib SDL2::SDL2main SDL2::SDL2 glad imgui)

target_include_directories(RayTracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${OPENGL_LIBRARIES}
)