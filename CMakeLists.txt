cmake_minimum_required(VERSION 3.28)

# CMAKE FLAGS
option(JTX_ENABLE_CUDA_BACKEND "Enable CUDA backend" OFF)
option(JTX_ENABLE_PROFILING "Enable profiling" ON)
option(JTX_ENABLE_PERF_FLAGS "Enable performance flags" ON)
option(JTX_ENABLE_MULTI_THREADING "Enable multi-threading" ON)
option(JTX_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(JTX_DISABLE_UI "Disable UI" OFF)
option(JTX_ENABLE_DEBUG_TRACES "Enable debug traces" ON)
option(JTX_USE_EMBREE "Enable embree acceleration structures" ON)

if (JTX_ENABLE_CUDA_BACKEND)
    project(JTX VERSION 1.0.0 LANGUAGES CXX CUDA)
else ()
    project(JTX VERSION 1.0.0 LANGUAGES CXX)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# PROCESS CMAKE FLAGS
if (JTX_ENABLE_CUDA_BACKEND)
    include(CheckLanguage)
    check_language(CUDA)

    if (CMAKE_CUDA_COMPILER)
        set(USE_CUDA ON CACHE BOOL "Use CUDA backend" FORCE)

        add_compile_definitions(-DJTX_ENABLE_CUDA_BACKEND)

        message(STATUS "[JTX] CUDA compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "[JTX] CUDA compiler version: ${CMAKE_CUDA_COMPILER_VERSION}")
    else ()
        set(USE_CUDA OFF CACHE BOOL "Use CUDA backend" FORCE)
        set(JTX_ENABLE_CUDA_BACKEND OFF CACHE BOOL "Enable CUDA backend" FORCE)
        message(STATUS "[JTX] CUDA compiler not found, continuing without CUDA support")
    endif ()
else ()
    message(STATUS "[JTX] CUDA support disabled")
endif ()

if (JTX_USE_EMBREE)
    add_compile_definitions(-DJTX_USE_EMBREE)

    if (WIN32)
        message(STATUS "[JTX] Windows detected -- loading vendored Embree")
        set(embree_DIR "ext/embree4/lib/cmake/embree-4.4.0" CACHE PATH "Embree CMake")
    endif()

    find_package(embree 4 REQUIRED)
    message(STATUS "[JTX] Embree ${embree_VERSION} found — building with Embree support")
else()
    message(STATUS "[JTX] Embree support is disabled")
endif()

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
        message(STATUS "[JTX] Using MSVC compiler")
        add_compile_options(/O2 /fp:fast)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "[JTX] Using GNU or Clang compiler")
        add_compile_options(-O3 -ffast-math)
    else ()
        message(WARNING "[JTX] Unknown compiler. No optimization flags added.")
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
# GLM (temporary)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ext/glm)

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

set (JTX_SCENE
        jtx2/scene/scene.hpp
        jtx2/scene/scene_loader.hpp
        jtx2/scene/scene_loader.cpp
)

set (JTX_UTIL
        jtx2/util/logger.hpp
        jtx2/util/color.hpp
        jtx2/util/aabb.hpp
        jtx2/util/rand.hpp
        jtx2/util/simd.hpp
)

set (JTX_UI
        jtx2/ui/display.hpp
        jtx2/ui/display.cpp
        jtx2/ui/ui_renderer.hpp
        jtx2/ui/ui_renderer.cpp
        jtx2/ui/rasterizer/rasterizer.hpp
        jtx2/ui/rasterizer/rasterizer.cpp
        jtx2/ui/rasterizer/camera.hpp
        jtx2/ui/rasterizer/camera.cpp
)

set (JTX_BACKEND_CPU
        jtx2/backends/cpu/bvh.hpp
        jtx2/backends/cpu/bvh.cpp
        jtx2/backends/cpu/rng.hpp
        jtx2/backends/cpu/bvh_test.hpp
)

add_executable(
        JTX
        ${JVK_SOURCE}
        ${JVK_HEADERS}
        ${JTX_SCENE}
        ${JTX_UTIL}
        ${JTX_UI}
        ${JTX_BACKEND_CPU}
        jtx2/main.cpp
        jtx2/jtx.hpp
        jtx2/material.h
        jtx2/image.hpp
        jtx2/image.cpp
        jtx2/util/timer.hpp
        jtx2/backends/cpu/isect.hpp
)


target_link_libraries(JTX PRIVATE
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        volk
        jtxlib
        SDL2::SDL2main
        SDL2::SDL2
        imgui
        fmt::fmt
        fastgltf::fastgltf
        glm
)

if (JTX_USE_EMBREE AND embree_FOUND)
    target_link_libraries(JTX PRIVATE embree)

    set(EMBREE_BIN_DIR "${CMAKE_SOURCE_DIR}/ext/embree4/bin")
    if(EXISTS "${EMBREE_BIN_DIR}")
        set(EMBREE_DLLS
                embree4.dll
                tbb12.dll
                tbbmalloc.dll
        )

        foreach(DLL_NAME ${EMBREE_DLLS})
            set(SOURCE_DLL "${EMBREE_BIN_DIR}/${DLL_NAME}")
            if(EXISTS "${SOURCE_DLL}")
                add_custom_command(TARGET JTX POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${SOURCE_DLL}"
                        $<TARGET_FILE_DIR:JTX>
                        COMMENT "Copying ${DLL_NAME} to output directory"
                )
            else()
                message(WARNING "[JTX] DLL not found for copying: ${SOURCE_DLL}")
            endif()
        endforeach()
    else()
        message(WARNING "[JTX] Embree bin directory not found at ${EMBREE_BIN_DIR}. DLLs will not be copied.")
    endif()
endif()

if (JTX_ENABLE_SIMD)
    message(STATUS "[JTX] SIMD optimizations requested -- querying intrinsics")

    include(CheckCXXCompilerFlag)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86|amd64|X86_64)$")
        check_cxx_compiler_flag("-msse4.1" HAVE_SSE4_1)
        if(HAVE_SSE4_1)
            message(STATUS "[JTX] Enabling SSE4.1 intrinsics")
            target_compile_options(${PROJECT_NAME} PRIVATE -msse4.1)
        else()
            message(WARNING "[JTX] Compiler does not support -msse4.1, SIMD disabled on x86")
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|arm64)$")
        check_cxx_compiler_flag("-mfpu=neon" HAVE_NEON)
        if(HAVE_NEON)
            message(STATUS "[JTX] Enabling NEON intrinsics")
            target_compile_options(${PROJECT_NAME} PRIVATE -mfpu=neon)
        else()
            message(WARNING "[JTX] Compiler does not support NEON flags, SIMD disabled on ARM")
        endif()

    else()
        message(WARNING "[JTX] Unknown architecture '${CMAKE_SYSTEM_PROCESSOR}', SIMD disabled")
    endif()
endif()

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
        COMMENT "[JTX] Copying assets directory to output folder"
)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/mesh.frag"
        "${PROJECT_SOURCE_DIR}/shaders/mesh.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "[JTX] Building shader")
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
