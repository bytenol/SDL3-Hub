cmake_minimum_required(VERSION 3.16)

project(SDL3-Hub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
# find_package(glm CONFIG REQUIRED)
# find_package(GTest REQUIRED)

add_subdirectory(small)
# add_subdirectory(test)