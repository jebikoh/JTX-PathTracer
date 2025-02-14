cmake_minimum_required(VERSION 3.28)

option(ENABLE_CUDA_BACKEND "Enable CUDA backend" OFF)
option(ENABLE_PROFILING "Enable profiling" ON)
option(ENABLE_PERF_FLAGS "Enable performance flags" ON)
option(ENABLE_MULTI_THREADING "Enable multi-threading" ON)
option(DISABLE_UI "Disable UI" OFF)

if (ENABLE_CUDA_BACKEND)
    project(JTX VERSION 1.0.0 LANGUAGES CXX CUDA)
else ()
    project(JTX VERSION 1.0.0 LANGUAGES CXX)
endif ()

set(CMAKE_CXX_STANDARD 20)

if (ENABLE_CUDA_BACKEND)
    include(CheckLanguage)
    check_language(CUDA)

    if (CMAKE_CUDA_COMPILER)
        set(USE_CUDA ON CACHE BOOL "Use CUDA backend" FORCE)

        add_compile_definitions(-DENABLE_CUDA_BACKEND)

        message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "CUDA compiler version: ${CMAKE_CUDA_COMPILER_VERSION}")
    else ()
        set(USE_CUDA OFF CACHE BOOL "Use CUDA backend" FORCE)
        set(ENABLE_CUDA_BACKEND OFF CACHE BOOL "Enable CUDA backend" FORCE)
        message(STATUS "CUDA compiler not found, continuing without CUDA support")
    endif ()
else ()
    message(STATUS "CUDA support disabled")
endif ()

if (ENABLE_PROFILING)
    add_compile_definitions(-DENABLE_PROFILING)
endif ()

if (ENABLE_MULTI_THREADING)
    add_compile_definitions(-DENABLE_MULTI_THREADING)
endif ()

if (DISABLE_UI)
    add_compile_definitions(-DDISABLE_UI)
endif ()

if (ENABLE_PERF_FLAGS)
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

add_subdirectory(ext/jtxlib)

add_subdirectory(ext/sdl EXCLUDE_FROM_ALL)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

add_subdirectory(ext/glad)

# assimp
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/assimp)

# ImGUI
set(IMGUI_PATH "ext/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_PATH})
target_link_libraries(imgui PRIVATE glad SDL2::SDL2)

add_executable(JTX
        src/main.cpp
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
        src/bsdf/conductor.hpp
        src/bsdf/dielectric.hpp
        src/bsdf/microfacet.hpp
        src/lights/lights.hpp
        src/bsdf/disney.hpp
)

target_link_libraries(JTX PRIVATE jtxlib SDL2::SDL2main SDL2::SDL2 glad imgui assimp)

target_include_directories(JTX
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyobjloader
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinygltf
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/IconsLucide
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/L2DFileDialog
        ${OPENGL_LIBRARIES}
)

if (WIN32)
    add_custom_command(TARGET jtxlib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:assimp>
            $<TARGET_FILE_DIR:jtxlib>
    )
endif ()

add_custom_command(TARGET JTX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/assets
        $<TARGET_FILE_DIR:JTX>/assets
        COMMENT "Copying assets directory to output folder"
)