cmake_minimum_required(VERSION 3.28)
project(jtxlib VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-O3 -mavx2)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Test" CACHE STRING "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_ASSERT)
elseif(CMAKE_BUILD_TYPE STREQUAL "Test")
    add_compile_definitions(TEST_ASSERT)
endif()

set(JTXLIB_MATH
        src/jtxlib/math/vec2.hpp
        src/jtxlib/math/vec3.hpp
        src/jtxlib/math/vec4.hpp
        src/jtxlib/math/vecmath.hpp
        src/jtxlib/math/transform.hpp
        src/jtxlib/math/spherical.hpp
        src/jtxlib/math/spherical.cpp
        src/jtxlib/math/ray.hpp
        src/jtxlib/math/numerical.hpp
        src/jtxlib/math/mat4.cpp
        src/jtxlib/math/mat4.hpp
        src/jtxlib/math/math.hpp
        src/jtxlib/math/bounds.hpp
        src/jtxlib/math/constants.hpp
)

set(JTXLIB_SIMD
        src/jtxlib/simd/avxfloat.hpp
)

set(JTXLIB_UTIL
        src/jtxlib/util/assert.hpp
)

set(JTXLIB_HEADERS
        src/jtxlib.hpp
        src/jtxlib/math.hpp
        src/jtxlib/simd.hpp
        src/jtxlib/util.hpp)

add_library(jtxlib STATIC
    ${JTXLIB_MATH}
    ${JTXLIB_SIMD}
    ${JTXLIB_UTIL}
    ${JTXLIB_HEADERS}
)

set_target_properties(jtxlib PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(jtxlib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

option(BUILD_TESTS "Build tests" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Test")
    add_subdirectory("lib/Catch2")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/Catch2/extras)
    include(CTest)
    include(Catch)

    add_subdirectory("tests")
endif()