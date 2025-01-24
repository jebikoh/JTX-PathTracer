cmake_minimum_required(VERSION 3.28)

option(ENABLE_CUDA_BACKEND "Enable CUDA backend" OFF)
option(ENABLE_PROFILING "Enable profiling" ON)
option(USE_DOUBLE_PRECISION "Use double precision" OFF)

if(ENABLE_CUDA_BACKEND)
    project(RayTracer VERSION 1.0.0 LANGUAGES CXX CUDA)
else()
    project(RayTracer VERSION 1.0.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)

if (ENABLE_CUDA_BACKEND)
    include(CheckLanguage)
    check_language(CUDA)

    if(CMAKE_CUDA_COMPILER)
        set(USE_CUDA ON CACHE BOOL "Use CUDA backend" FORCE)

        add_compile_definitions(-DENABLE_CUDA_BACKEND)

        message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "CUDA compiler version: ${CMAKE_CUDA_COMPILER_VERSION}")
    else()
        set(USE_CUDA OFF CACHE BOOL "Use CUDA backend" FORCE)
        set(ENABLE_CUDA_BACKEND OFF CACHE BOOL "Enable CUDA backend" FORCE)
        message(STATUS "CUDA compiler not found, continuing without CUDA support")
    endif()
else()
    message(STATUS "CUDA support disabled")
endif()

if (ENABLE_PROFILING)
    add_compile_definitions(-DENABLE_PROFILING)
endif()

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
        src/util/color.hpp
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
        src/util/interval.hpp
        src/util/interval.cpp
        src/util/aabb.hpp
        src/util/aabb.cpp
        src/bvh.cpp
        src/sampling.hpp
        src/util/hash.hpp
        src/filter.hpp
        src/mesh.hpp
        src/integrator.hpp
        src/integrator.cpp
        src/util/complex.hpp
        src/bsdf/diffuse.hpp
        src/bsdf/bxdf.hpp
        src/bsdf/bxdf.cpp
)

target_link_libraries(RayTracer PRIVATE jtxlib SDL2::SDL2main SDL2::SDL2 glad imgui)

target_include_directories(RayTracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyobjloader
        ${OPENGL_LIBRARIES}
)