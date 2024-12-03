cmake_minimum_required(VERSION 3.28)
project(RayTracer)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_TESTS OFF CACHE BOOL "Build tests for jtxlib" FORCE)
add_subdirectory(ext/jtxlib)

add_executable(RayTracer src/main.cpp
        src/rt.hpp
        src/color.hpp)

target_link_libraries(RayTracer PRIVATE jtxlib)

target_include_directories(RayTracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
)