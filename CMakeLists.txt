cmake_minimum_required(VERSION 3.28)
project(RayTracer)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_TESTS OFF CACHE BOOL "Build tests for jtxlib" FORCE)
set(NDEBUG OFF CACHE BOOL "Disable debug assertions" FORCE)

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
        src/hittable.hpp
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
)

target_link_libraries(RayTracer PRIVATE jtxlib SDL2::SDL2main SDL2::SDL2 glad imgui)

target_include_directories(RayTracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${OPENGL_LIBRARIES}
)