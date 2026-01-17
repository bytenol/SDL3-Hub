cmake_minimum_required(VERSION 3.10)

project(SDL3-Hub-Small)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(SDL3_image CONFIG REQUIRED)

add_library(ideps INTERFACE)

# target_link_libraries(ideps INTERFACE SDL3::SDL3 SDL3_image::SDL3_image glm::glm)
target_link_libraries(ideps INTERFACE SDL3::SDL3)

# add_executable(transformation transformation.cpp)
# target_link_libraries(transformation PRIVATE ideps)

# add_executable(tictactoe tictactoe.cpp)
# target_link_libraries(tictactoe PRIVATE ideps)

# add_executable(integrationScheme integrationScheme.cpp)
# target_link_libraries(integrationScheme PRIVATE ideps)

# add_executable(verlet verlet.cpp)
# target_link_libraries(verlet PRIVATE ideps)

# add_executable(SAT SAT.cpp)
# target_link_libraries(SAT PRIVATE ideps)

add_executable(raycasting raycasting.cpp)
target_link_libraries(raycasting PRIVATE ideps)

add_executable(pong2d pong2d.cpp)
target_link_libraries(pong2d PRIVATE ideps)

add_executable(tetris tetris.cpp)
target_link_libraries(tetris PRIVATE ideps)

add_executable(archimedes archimedes.cpp)
target_link_libraries(archimedes PRIVATE ideps)

add_executable(sierpienskiTriangle sierpienskiTriangle.cpp)
target_link_libraries(sierpienskiTriangle PRIVATE ideps)

add_executable(rigidBody1 rigidBody1.cpp)
target_link_libraries(rigidBody1 PRIVATE ideps)

add_executable(rigidBody2 rigidBody2.cpp)
target_link_libraries(rigidBody2 PRIVATE ideps)

add_executable(rigidPhysics rigidPhysics.cpp)
target_link_libraries(rigidPhysics PRIVATE ideps)


add_executable(polygonCollision polygonCollision.cpp)
target_link_libraries(polygonCollision PRIVATE ideps)