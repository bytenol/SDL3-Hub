cmake_minimum_required(VERSION 3.10)

project(SDL3-Hub-Physics)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bouncingBall bouncingBall.cpp)
target_link_libraries(bouncingBall PRIVATE SDL3::SDL3)