cmake_minimum_required(VERSION 3.28)
project(rtx)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ext/jtxlib)

add_executable(rtx src/main.cpp
        src/color.hpp
        src/hittable.hpp
        src/sphere.hpp
        src/hittablelist.hpp
        src/rtx.hpp
        src/interval.hpp
        src/camera.hpp)

target_link_libraries(rtx PRIVATE jtxlib)
target_link_options(rtx PRIVATE -static)

target_include_directories(rtx
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/jtxlib/src
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(rtx PRIVATE DEBUG_ASSERT)
elseif (CMAKE_BUILD_TYPE STREQUAL "Test")
    target_compile_definitions(rtx PRIVATE TEST_ASSERT)
endif ()