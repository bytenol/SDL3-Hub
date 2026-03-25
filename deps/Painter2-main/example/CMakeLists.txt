project(painter2_example)

add_executable(sdf sdf.cpp)
target_link_libraries(sdf PRIVATE painter2 glfw)

add_executable(window window.cpp)
target_link_libraries(window PRIVATE painter2 glfw)