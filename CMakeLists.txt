
cmake_minimum_required(VERSION 3.19)

project(Teide VERSION 0.0.0
              DESCRIPTION "Vulkan-based 3D renderer"
              LANGUAGES CXX)

add_executable(Teide src/main.cpp)
target_compile_features(Teide PUBLIC cxx_std_20)

find_package(Vulkan REQUIRED)
target_link_libraries(Teide PRIVATE Vulkan::Vulkan)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(Teide PRIVATE SDL2::SDL2 SDL2::SDL2main)

