cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(tools/build/Build.cmake)
include(tools/build/RenderDoc.cmake)
include(tools/build/Swiftshader.cmake)

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

option(TEIDE_EXAMPLES "Build examples for Teide" OFF)

enable_testing()

find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(function2 CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(vulkan_options VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

include(FileLists.cmake)
td_add_library(
    Teide
    SOURCES "${sources}"
    PUBLIC_DEPS spdlog::spdlog
    PRIVATE_DEPS
        Vulkan::Vulkan
        SDL2::SDL2
        fmt::fmt
        Taskflow::Taskflow
        function2::function2
        glslang::HLSL
        glslang::SPIRV
        glslang::glslang
        glslang::OGLCompiler)

target_compile_definitions(Teide PRIVATE ${vulkan_options})

if(BUILD_TESTING AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    install_swiftshader()
    find_renderdoc()

    add_subdirectory(tests)
endif()

if(TEIDE_EXAMPLES)
    add_subdirectory(examples)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)
endif()
