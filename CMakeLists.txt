cmake_minimum_required(VERSION 3.10)

project(SDL3-Hub)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED NO)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest REQUIRED)

include_directories("C:/Extract/C++/vcpkg/installed/x64-windows/include")

#message(C:\Extract\C++\vcpkg\installed\x64-windows\include)

add_subdirectory(small)
add_subdirectory("test")
