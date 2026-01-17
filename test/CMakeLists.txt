cmake_minimum_required(VERSION 3.10)

project(SDL3-Hub-Test)

add_library(cdeps INTERFACE)

target_link_libraries(cdeps INTERFACE SDL3::SDL3 GTest::gtest GTest::gtest_main)


add_executable(window_test window_test.cpp)
target_link_libraries(window_test PRIVATE cdeps)