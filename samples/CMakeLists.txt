cmake_minimum_required(VERSION 3.29)

project(samples)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(test_triangle)
add_subdirectory(test_scene)
add_subdirectory(test_lights)
add_subdirectory(test_sponza)
add_subdirectory(test_pbr)