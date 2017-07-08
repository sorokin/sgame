cmake_minimum_required(VERSION 3.4)

project(sgame)

set(CMAKE_CXX_STANDARD 14)

add_library(sg STATIC simple_game_window.h simple_game_window.cpp)

add_executable(house house_demo.cpp)
add_executable(circles circles_demo.cpp)
add_executable(snake snake_demo.cpp)

target_link_libraries(sg GL GLU SDL2 cairo)

target_link_libraries(house sg)
target_link_libraries(circles sg)
target_link_libraries(snake sg)
