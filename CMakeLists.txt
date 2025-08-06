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

# ──────────────────────────────────────────────────────────────────────────────
#  Project declaration
# ──────────────────────────────────────────────────────────────────────────────
project(JTX VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ──────────────────────────────────────────────────────────────────────────────
#  Compiler definitions & options
# ──────────────────────────────────────────────────────────────────────────────
if (JTX_ENABLE_PROFILING)
    add_compile_definitions(-DJTX_ENABLE_PROFILING)
endif ()

if (JTX_ENABLE_PERF_FLAGS)
    if (MSVC)
        message(STATUS "[JTX] Using MSVC compiler")
        add_compile_options(/O2)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "[JTX] Using GNU or Clang compiler")
        add_compile_options(-O3)
    else ()
        message(WARNING "[JTX] Unknown compiler. No optimization flags added.")
    endif ()
endif ()

if (JTX_ENABLE_MULTI_THREADING)
    add_compile_definitions(-DJTX_ENABLE_MULTI_THREADING)
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Third‑party dependencies
# ──────────────────────────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)

# Vendored ─────────────────────────────────────────────────────────────────────
add_subdirectory(ext/volk)                   # Volk
add_subdirectory(ext/vkbootstrap)            # VkBootstrap
add_subdirectory(ext/vma)                    # VMA
add_subdirectory(ext/jtxlib)                 # JTXLib
add_subdirectory(ext/fastgltf)               # fastglTF

# SDL3
include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG 68bfcb6c5419f51104e74e72ea0f8d405a4615b0 # 3.2.18
)
FetchContent_MakeAvailable(SDL3)

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
        ext/imgui/backends/imgui_impl_vulkan.cpp
        ext/imgui/backends/imgui_impl_sdl3.cpp
)
target_compile_definitions(imgui
        PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK
)
target_link_libraries(imgui PUBLIC volk Vulkan::Vulkan SDL3::SDL3)

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

# Native File Dialog Extended
add_subdirectory(ext/nfd)

# fmt
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

if (JTX_ENABLE_PROFILING)
    FetchContent_Declare (
            tracy
            GIT_REPOSITORY https://github.com/wolfpld/tracy.git
            GIT_TAG master
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
    )
    option (TRACY_ENABLE "" ON )
    FetchContent_MakeAvailable (tracy)
endif ()

# ──────────────────────────────────────────────────────────────────────────────
#  Source file groups
# ──────────────────────────────────────────────────────────────────────────────
SET(JVK_HEADERS
        jtx2/jvk/buffer.hpp
        jtx2/jvk/commands.hpp
        jtx2/jvk/context.hpp
        jtx2/jvk/descriptor.hpp
        jtx2/jvk/fence.hpp
        jtx2/jvk/image.hpp
        jtx2/jvk/init.hpp
        jtx2/jvk/jvk.hpp
        jtx2/jvk/pipeline.hpp
        jtx2/jvk/queue.hpp
        jtx2/jvk/sampler.hpp
        jtx2/jvk/semaphore.hpp
        jtx2/jvk/shaders.hpp
        jtx2/jvk/swapchain.hpp
        jtx2/jvk/util.hpp
        jtx2/jvk/immediate_buffer.hpp
)

set(JVK_SOURCE
        jtx2/jvk/descriptor.cpp
        jtx2/jvk/pipeline.cpp
        jtx2/jvk/util.cpp
)

set(JTX_SCENE
        jtx2/scene/scene.hpp
        jtx2/scene/scene_loader.hpp
        jtx2/scene/scene_loader.cpp
        jtx2/scene/scene_exporter.hpp
        jtx2/scene/scene_exporter.cpp
)

set(JTX_UTIL
        jtx2/util/logger.hpp
        jtx2/util/color.hpp
        jtx2/util/aabb.hpp
        jtx2/util/rand.hpp
        jtx2/util/simd.hpp
        jtx2/util/sampling.hpp
        jtx2/util/profiling.hpp
        jtx2/util/complex.hpp
)

set(JTX_INTERFACE
        jtx2/editor/editor.hpp
        jtx2/editor/editor.cpp
        jtx2/editor/ui_renderer.hpp
        jtx2/editor/ui_renderer.cpp
        jtx2/editor/gfx_context.cpp
        jtx2/editor/gfx_context.hpp
)

set(JTX_BVH
        jtx2/bvh/bvh.hpp
        jtx2/bvh/bvh.cpp
        jtx2/bvh/isect.hpp
)

set(JTX_BACKEND_CPU
        jtx2/engine/cpu/camera.hpp
        jtx2/engine/cpu/backend_cpu.hpp
        jtx2/engine/cpu/backend_cpu.cpp
        jtx2/engine/cpu/integrator.hpp
        jtx2/engine/cpu/integrator.cpp
        jtx2/engine/cpu/bxdf/diffuse.hpp
        jtx2/engine/cpu/bxdf/bxdf.hpp
        jtx2/engine/cpu/bxdf/bxdf.cpp
        jtx2/engine/cpu/bxdf/microfacet.hpp
        jtx2/engine/cpu/bxdf/conductor.hpp
        jtx2/engine/cpu/bxdf/dielectric.hpp
        jtx2/engine/cpu/bxdf/glossydiffuse.hpp
        jtx2/engine/cpu/bxdf/thin_dielectric.hpp
)

set(JTX_BACKEND
        jtx2/engine/backends.hpp
)

set(JTX_WING
        jtx2/engine/vulkan/vk_engine.hpp
        jtx2/engine/vulkan/vk_engine.cpp
        jtx2/engine/vulkan/camera.hpp
        jtx2/engine/vulkan/render_data.hpp
        jtx2/engine/vulkan/accel.hpp
        jtx2/engine/vulkan/accel.cpp
        jtx2/engine/cpu/energy_compensation.hpp
        jtx2/engine/cpu/bxdf/oren_nayar.hpp
)

# ──────────────────────────────────────────────────────────────────────────────
#  Static library: JTXCore
# ──────────────────────────────────────────────────────────────────────────────
add_library(JTXCore STATIC
        ${JVK_SOURCE}
        ${JVK_HEADERS}
        ${JTX_SCENE}
        ${JTX_UTIL}
        ${JTX_INTERFACE}
        ${JTX_BVH}
        ${JTX_BACKEND_CPU}
        ${JTX_BACKEND}
        ${JTX_WING}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/rapidjson
)

if(APPLE)
    target_link_libraries(JTXCore PRIVATE
            "-framework AppKit"
            "-framework UniformTypeIdentifiers"
    )
elseif(WIN32)
    target_link_libraries(JTXCore PRIVATE
            ole32
            uuid
            shell32
    )
endif()

target_link_libraries(JTXCore PUBLIC
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        volk
        jtxlib
        SDL3::SDL3
        imgui
        fmt::fmt
        fastgltf::fastgltf
        glm
        nfd
)

if (JTX_ENABLE_PROFILING)
    target_link_libraries(JTXCore PUBLIC Tracy::TracyClient)
endif()

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
#  Executables
# ──────────────────────────────────────────────────────────────────────────────
add_executable(JTX jtx2/main.cpp)
target_link_libraries(JTX PRIVATE JTXCore)

add_executable(JTXEditor jtx2/editor_main.cpp)
target_link_libraries(JTXEditor PRIVATE JTXCore)

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
            ${CMAKE_BINARY_DIR}/assets
            COMMENT "[JTX] Copying assets directory to output folder"
    )
endfunction()
jtx_post_build(JTX)
jtx_post_build(JTXEditor)

# ──────────────────────────────────────────────────────────────────────────────
#  Unit tests
# ──────────────────────────────────────────────────────────────────────────────
if (JTX_BUILD_TESTS)
    message(STATUS "[JTX] Building tests")
    set(JTX_TEST_SOURCES
            jtx2/tests/bvh_test.cpp
            jtx2/tests/aabb_test.cpp
            jtx2/tests/sampling_test.cpp
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
set(SPIRV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/spv")
file(MAKE_DIRECTORY "${SPIRV_OUTPUT_DIR}")

# TEMPORARY UNTIL ALL GLSL FILES ARE REMOVED
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "[JTX] Slang compiler (slangc) not found.")
endif()
message(STATUS "[JTX] Found GLSL compiler: ${GLSL_VALIDATOR}")

find_program(SLANG_COMPILER slangc HINTS $ENV{SLANG_DIR}/bin/ $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
if(NOT SLANG_COMPILER)
    message(FATAL_ERROR "[JTX] Slang compiler (slangc) not found.")
endif()
message(STATUS "[JTX] Found Slang compiler: ${SLANG_COMPILER}")


set(GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/grid.frag"
        "${PROJECT_SOURCE_DIR}/shaders/grid.vert"
)

foreach (GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "[JTX] GLSL Shader: ${GLSL}")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SPIRV_OUTPUT_DIR}/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.2 ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

set(SLANG_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/mesh.slang"
)

set(SLANG_COMMON_MODULES
    "${PROJECT_SOURCE_DIR}/shaders/common/bxdf.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/color.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/constants.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/data.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/math.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/random.slang"
    "${PROJECT_SOURCE_DIR}/shaders/common/sampling.slang"
)

function(compile_slang_shader SOURCE_FILE ENTRY_POINT OUTPUT_VAR)
    get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME_WE)

    set(OUTPUT_SPIRV "${SPIRV_OUTPUT_DIR}/${SOURCE_NAME}_${ENTRY_POINT}.spv")
    set(OUTPUT_DEP_FILE "${SPIRV_OUTPUT_DIR}/${SOURCE_NAME}_${ENTRY_POINT}.spv.d")
    add_custom_command(
            OUTPUT ${OUTPUT_SPIRV}
            COMMAND ${SLANG_COMPILER}
            ${SOURCE_FILE}
            -profile glsl_450
            -target spirv
            -entry ${ENTRY_POINT}
            -o ${OUTPUT_SPIRV}
            DEPENDS ${SOURCE_FILE} ${SLANG_COMMON_MODULES}
            COMMENT "Compiling Slang shader: ${SOURCE_FILE} -> ${ENTRY_POINT}"
    )
    message(STATUS "[JTX] Slang Shader: ${SOURCE_NAME}::${ENTRY_POINT}")
    set(${OUTPUT_VAR} ${OUTPUT_SPIRV} PARENT_SCOPE)
endfunction()

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/mesh.slang" vertexMain MESH_VERTEX_SPV)
list(APPEND SPIRV_BINARY_FILES ${MESH_VERTEX_SPV})

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/mesh.slang" fragmentMain MESH_FRAG_SPV)
list(APPEND SPIRV_BINARY_FILES ${MESH_FRAG_SPV})

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/raytrace.slang" rayGenMain RT_RAYGEN_SPV)
list(APPEND SPIRV_BINARY_FILES ${RT_RAYGEN_SPV})

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/raytrace.slang" closestHitMain RT_CLOSEST_SPV)
list(APPEND SPIRV_BINARY_FILES ${RT_CLOSEST_SPV})

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/raytrace.slang" missMain RT_MISS_SPV)
list(APPEND SPIRV_BINARY_FILES ${RT_MISS_SPV})

compile_slang_shader("${PROJECT_SOURCE_DIR}/shaders/postprocessing.slang" computeMain POST_FX_SPV)
list(APPEND SPIRV_BINARY_FILES ${POST_FX_SPV})

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(JTX Shaders)
add_dependencies(JTXEditor Shaders)

# ──────────────────────────────────────────────────────────────────────────────
#  Packaging
# ──────────────────────────────────────────────────────────────────────────────
INSTALL(TARGETS JTXEditor RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/jtx2/assets DESTINATION .)
if (JTX_USE_EMBREE AND embree_FOUND AND WIN32)
    set(EMBREE_BIN_DIR "${CMAKE_SOURCE_DIR}/ext/embree4/bin")
    install(FILES
            "${EMBREE_BIN_DIR}/embree4.dll"
            "${EMBREE_BIN_DIR}/tbb12.dll"
            "${EMBREE_BIN_DIR}/tbbmalloc.dll"
            DESTINATION bin
            COMPONENT Runtime
    )
endif()
install(DIRECTORY ${SPIRV_OUTPUT_DIR}/ DESTINATION bin/spv)

include(CPack)
