cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(tools/Build.cmake)

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

option(TEIDE_EXAMPLES "Build examples for Teide" OFF)

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

if(MSVC)
    # Current Visual Studio versions require /std:c++latest for some C++20 features, which requires cxx_std_23
    set(cxx_standard cxx_std_23)
else()
    set(cxx_standard cxx_std_20)
endif()

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

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        td_add_test(
            TeideTest
            SOURCES "${test_sources}"
            DEPS Teide
                 SDL2::SDL2
                 Vulkan::Vulkan
                 GTest::gtest
                 GTest::gmock)
        target_compile_definitions(TeideTest PRIVATE ${vulkan_options})
    endif()
endif()

if(TEIDE_EXAMPLES)
    add_subdirectory(examples/Application)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)
endif()
