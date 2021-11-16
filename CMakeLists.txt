
cmake_minimum_required(VERSION 3.19)

project(Teide VERSION 0.0.0
              DESCRIPTION "Vulkan-based 3D renderer"
              LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(gtest CONFIG)
find_path(STB_INCLUDE_DIRS "stb.h")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Teide)

include(CTest)
enable_testing()

# ----- GeoLib -----

set(GeoLibInclude source)
set(sources
    ${GeoLibInclude}/GeoLib/Vector.h
    ${GeoLibInclude}/GeoLib/Matrix.h
    ${GeoLibInclude}/GeoLib/Angle.h
)

add_library(GeoLib INTERFACE ${sources})
target_include_directories(GeoLib INTERFACE ${GeoLibInclude})
target_compile_features(GeoLib INTERFACE cxx_std_23)

# ----- GeoLibTest -----

set(sources
    test/GeoLib/MatrixTest.cpp
    test/GeoLib/VectorTest.cpp
)

add_executable(GeoLibTest ${sources})
target_link_libraries(GeoLibTest PRIVATE GeoLib GTest::gtest GTest::gtest_main)
add_test(NAME GeoLibTest COMMAND GeoLibTest)

# ----- Application -----

set(sources
    source/VulkanApp.cpp
    source/ShaderCompiler.h
    source/ShaderCompiler.cpp
    source/Definitions.h
)

add_executable(Teide WIN32 ${sources})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
target_compile_features(Teide PUBLIC cxx_std_23)
target_compile_definitions(Teide PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
set_property(TARGET Teide PROPERTY VS_DPI_AWARE "PerMonitor")

target_link_libraries(Teide PRIVATE
    GeoLib
    Vulkan::Vulkan
    SDL2::SDL2 SDL2::SDL2main
    spdlog::spdlog
    HLSL SPIRV glslang OGLCompiler
    assimp::assimp
)
target_include_directories(Teide PRIVATE ${STB_INCLUDE_DIRS})
