cmake_minimum_required(VERSION 3.28)

# ──────────────────────────────────────────────────────────────────────────────
#  Project‑wide options
# ──────────────────────────────────────────────────────────────────────────────
option(JTX_ENABLE_PROFILING "Enable profiling" ON)
option(JTX_ENABLE_PERF_FLAGS "Enable performance flags" ON)
option(JTX_ENABLE_MULTI_THREADING "Enable multi‑threading" ON)
option(JTX_ENABLE_SIMD "Enable SIMD optimisations" ON)
option(JTX_USE_EMBREE "Enable Embree acceleration structures" ON)

option(JTX_BUILD_TESTS "Build unit tests" OFF)
option(JTX_BUILD_BENCHMARKS "Build performance benches" OFF)

option(_JTX_SUPPRESS_DEPENDENCY_MESSAGES "Suppress status messages from deps" ON)

# ──────────────────────────────────────────────────────────────────────────────
#  Project declaration
# ──────────────────────────────────────────────────────────────────────────────
project(JTX VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ──────────────────────────────────────────────────────────────────────────────
#  Compiler definitions & options
# ──────────────────────────────────────────────────────────────────────────────
if (JTX_ENABLE_PROFILING)
    add_compile_definitions(-DJTX_ENABLE_PROFILING)
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

if (JTX_ENABLE_MULTI_THREADING)
    add_compile_definitions(-DJTX_ENABLE_MULTI_THREADING)
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Helpers
# ──────────────────────────────────────────────────────────────────────────────
function(message)
    if (NOT MESSAGE_QUIET)
        _message(${ARGN})
    endif ()
endfunction()

if (_JTX_SUPPRESS_DEPENDENCY_MESSAGES)
    set(MESSAGE_QUIET ON)
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Third‑party dependencies
# ──────────────────────────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)

# Vendored ─────────────────────────────────────────────────────────────────────
add_subdirectory(ext/volk)                   # Volk
add_subdirectory(ext/vkbootstrap)            # VkBootstrap
add_subdirectory(ext/vma)                    # VMA
add_subdirectory(ext/sdl EXCLUDE_FROM_ALL)   # SDL2
add_subdirectory(ext/jtxlib)                 # JTXLib
add_subdirectory(ext/fastgltf)               # fastglTF

# ImGui
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

# GLM
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ext/glm)

# Embree
# Only vendored for windows; for macOS/Linux, it is expected to be installed via PM
if (JTX_USE_EMBREE)
    add_compile_definitions(-DJTX_USE_EMBREE)

    if (WIN32)
        message(STATUS "[JTX] Windows detected -- loading vendored Embree")
        set(embree_DIR "ext/embree4/lib/cmake/embree-4.4.0" CACHE PATH "Embree CMake")
    endif ()

    find_package(embree 4 REQUIRED)
    message(STATUS "[JTX] Embree ${embree_VERSION} found — building with Embree support")
else ()
    message(STATUS "[JTX] Embree support is disabled")
endif ()

# Fetched ─────────────────────────────────────────────────────────────────────
# fmt
include(FetchContent)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

# gtest (optional)
if (JTX_BUILD_TESTS)
    message(STATUS "[JTX] Fetching gtest")
    add_compile_definitions(-DJTX_BUILD_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

# Google benchmark (optional)
if (JTX_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG main)
    FetchContent_MakeAvailable(benchmark)
endif ()

if (_JTX_SUPPRESS_DEPENDENCY_MESSAGES)
    set(MESSAGE_QUIET OFF)
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Source file groups
# ──────────────────────────────────────────────────────────────────────────────
SET(JVK_HEADERS
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
        jtx2/ui/jvk/immediate_buffer.hpp
        jtx2/ui/rasterizer/render_data.hpp
        jtx2/backends/backends.hpp
)

set(JVK_SOURCE
        jtx2/ui/jvk/descriptor.cpp
        jtx2/ui/jvk/pipeline.cpp
        jtx2/ui/jvk/util.cpp
)

set(JTX_SCENE
        jtx2/scene/scene.hpp
        jtx2/scene/scene_loader.hpp
        jtx2/scene/scene_loader.cpp
)

set(JTX_UTIL
        jtx2/util/logger.hpp
        jtx2/util/color.hpp
        jtx2/util/aabb.hpp
        jtx2/util/rand.hpp
        jtx2/util/simd.hpp
        jtx2/util/rng.hpp
        jtx2/util/timer.hpp
)

set(JTX_UI
        jtx2/ui/display.hpp
        jtx2/ui/display.cpp
        jtx2/ui/ui_renderer.hpp
        jtx2/ui/ui_renderer.cpp
        jtx2/ui/rasterizer/rasterizer.hpp
        jtx2/ui/rasterizer/rasterizer.cpp
        jtx2/ui/rasterizer/camera.hpp
        jtx2/ui/rasterizer/camera.cpp
)

set(JTX_BACKEND_CPU
        jtx2/backends/cpu_pt/bvh.hpp
        jtx2/backends/cpu_pt/bvh.cpp
        jtx2/backends/cpu_pt/isect.hpp
        jtx2/backends/cpu_pt/camera.hpp
        jtx2/backends/cpu_pt/integrator.hpp
        jtx2/backends/cpu_pt/integrator.cpp
)

# ──────────────────────────────────────────────────────────────────────────────
#  Static library: JTXCore
# ──────────────────────────────────────────────────────────────────────────────
add_library(JTXCore STATIC
        ${JVK_SOURCE}
        ${JVK_HEADERS}
        ${JTX_SCENE}
        ${JTX_UTIL}
        ${JTX_UI}
        ${JTX_BACKEND_CPU}
        jtx2/jtx.hpp
        jtx2/material.h
        jtx2/image.hpp
        jtx2/image.cpp)

target_include_directories(JTXCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/jtx2
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/rapidobj
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/IconsLucide
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/imfilebrowser
)

target_link_libraries(JTXCore PUBLIC
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
    target_link_libraries(JTXCore PUBLIC embree)
endif ()

# SIMD compile flags/macros ----------------------------------------------------
if (JTX_ENABLE_SIMD)
    message(STATUS "[JTX] SIMD optimizations requested -- querying intrinsics")

    include(CheckCXXCompilerFlag)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86|amd64|X86_64|AMD64)$")
        if (MSVC)
            message(STATUS "[JTX] MSVC: SSE4.2 intrinsics enabled")
            target_compile_definitions(JTXCore PUBLIC JTX_SIMD_X86_SSE4_2)
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            check_cxx_compiler_flag("-msse4.2" HAVE_SSE4_2)
            if (HAVE_SSE4_2)
                message(STATUS "[JTX] GCC/Clang: enabling SSE4.2 intrinsics")
                target_compile_options(JTXCore PUBLIC -msse4.2)
                target_compile_definitions(JTXCore PUBLIC JTX_SIMD_X86_SSE4_2)
            else ()
                message(WARNING "[JTX] GCC/Clang: compiler does not support -msse4.2, SIMD disabled on x86")
            endif ()
        endif ()
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|arm64)$")
        check_cxx_compiler_flag("-mfpu=neon" HAVE_NEON)
        if (HAVE_NEON)
            message(STATUS "[JTX] Enabling NEON intrinsics")
            target_compile_options(JTXCore PUBLIC -mfpu=neon)
            target_compile_definitions(JTXCore PUBLIC JTX_SIMD_ARM_NEON)
        else ()
            message(WARNING "[JTX] Compiler does not support NEON flags, SIMD disabled on ARM")
        endif ()

    else ()
        message(WARNING "[JTX] Unknown architecture '${CMAKE_SYSTEM_PROCESSOR}', SIMD disabled")
    endif ()
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Main executable
# ──────────────────────────────────────────────────────────────────────────────
add_executable(JTX jtx2/main.cpp)
target_link_libraries(JTX PRIVATE JTXCore)

# ──────────────────────────────────────────────────────────────────────────────
#  Post-build
# ──────────────────────────────────────────────────────────────────────────────
function(jtx_post_build target)
    # Embree DLLs for Windows ──────────────────────────────────────────────────────
    if (JTX_USE_EMBREE AND embree_FOUND AND WIN32)
        set(EMBREE_BIN_DIR "${CMAKE_SOURCE_DIR}/ext/embree4/bin")
        if (EXISTS "${EMBREE_BIN_DIR}" AND WIN32)
            set(EMBREE_DLLS
                    embree4.dll
                    tbb12.dll
                    tbbmalloc.dll
            )

            foreach (DLL_NAME ${EMBREE_DLLS})
                set(SOURCE_DLL "${EMBREE_BIN_DIR}/${DLL_NAME}")
                if (EXISTS "${SOURCE_DLL}")
                    add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${SOURCE_DLL}"
                            $<TARGET_FILE_DIR:${target}>
                            COMMENT "[JTX] Copying ${DLL_NAME} to output directory"
                    )
                else ()
                    message(WARNING "[JTX] DLL not found for copying: ${SOURCE_DLL}")
                endif ()
            endforeach ()
        endif ()
    endif ()

    # Copy assets post‑build ───────────────────────────────────────────────────────
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/jtx2/assets
            $<TARGET_FILE_DIR:${target}>/assets
            COMMENT "[JTX] Copying assets directory to output folder"
    )
endfunction()
jtx_post_build(JTX)

# ──────────────────────────────────────────────────────────────────────────────
#  Unit tests
# ──────────────────────────────────────────────────────────────────────────────
if (JTX_BUILD_TESTS)
    message(STATUS "[JTX] Building tests")
    set(JTX_TEST_SOURCES
            jtx2/backends/cpu_pt/bvh_test.cpp
            jtx2/util/aabb_test.cpp
    )

    add_executable(JTXTests ${JTX_TEST_SOURCES})
    target_link_libraries(JTXTests PRIVATE JTXCore GTest::gtest_main)
    jtx_post_build(JTXTests)
    include(GoogleTest)
    gtest_discover_tests(JTXTests)
endif()

# ──────────────────────────────────────────────────────────────────────────────
#  Benchmarks
# ──────────────────────────────────────────────────────────────────────────────
if (JTX_BUILD_BENCHMARKS)
    message(STATUS "[JTX] Building benchmarks")
    function(add_benchmark BENCH_NAME)
        add_executable(${BENCH_NAME} ${ARGN})
        target_link_libraries(${BENCH_NAME} PRIVATE
                JTXCore
                benchmark::benchmark benchmark::benchmark_main)
        jtx_post_build(${BENCH_NAME})
    endfunction()
    set(BENCH_SRC
            jtx2/benchmarks/aabb_bm.cpp
            jtx2/benchmarks/bvh_bm.cpp
            jtx2/benchmarks/rng_bm.cpp
    )
    foreach(src ${BENCH_SRC})
        get_filename_component(bname ${src} NAME_WE)
        add_benchmark(${bname} ${src})
    endforeach()
endif()

# ──────────────────────────────────────────────────────────────────────────────
#  Shader compilation
# ──────────────────────────────────────────────────────────────────────────────
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/mesh.frag"
        "${PROJECT_SOURCE_DIR}/shaders/mesh.vert"
)

foreach (GLSL ${GLSL_SOURCE_FILES})
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
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
