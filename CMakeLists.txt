
cmake_minimum_required(VERSION 3.19)

project(Teide VERSION 0.0.0
              DESCRIPTION "Vulkan-based 3D renderer"
              LANGUAGES CXX)

set(sources
    source/VulkanApp.cpp
    source/ShaderCompiler.h
    source/ShaderCompiler.cpp
    source/Definitions.h
    source/GeoLib/Vector.h
    source/GeoLib/Matrix.h
    source/GeoLib/Angle.h
)

add_executable(Teide WIN32 ${sources})
source_group(TREE source PREFIX Source FILES ${sources})
target_compile_features(Teide PUBLIC cxx_std_23)
target_compile_definitions(Teide PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
set_property(TARGET Teide PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Teide)

find_package(Vulkan REQUIRED)
target_link_libraries(Teide PRIVATE Vulkan::Vulkan)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(Teide PRIVATE SDL2::SDL2 SDL2::SDL2main)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(Teide PRIVATE spdlog::spdlog)

find_package(glslang CONFIG REQUIRED)
target_link_libraries(Teide PRIVATE HLSL SPIRV glslang OGLCompiler)
