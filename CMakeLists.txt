cmake_minimum_required(VERSION 3.10)

project(ScreenTool)

set(CMAKE_CXX_STANDARD 17)

add_executable(ScreenTool src/main.cpp)

target_link_libraries(ScreenTool gdi32)
