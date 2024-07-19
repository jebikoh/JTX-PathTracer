cmake_minimum_required(VERSION 3.28)
project(rtx)

set(CMAKE_CXX_STANDARD 17)

add_executable(rtx main.cpp)

target_link_libraries(rtx -static)
