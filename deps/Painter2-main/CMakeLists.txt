cmake_minimum_required(VERSION 3.19)

project(painter2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSION OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

include_directories(deps/glad/include)
include_directories(include)

add_library(painter2 STATIC 
    "deps/glad/include/glad/glad.h"
    "deps/glad/src/glad.c"
    "src/buffer.cpp"
    "src/shapes.cpp"
    "src/core.cpp"
)

target_link_libraries(painter2 PRIVATE glfw)

add_subdirectory(example)