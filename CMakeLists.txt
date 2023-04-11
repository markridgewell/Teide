cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(tools/cmake/Build.cmake)

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

option(TEIDE_BUILD_EXAMPLES "Build examples for Teide" ON)
option(TEIDE_BUILD_TESTS "Build tests for Teide" ON)
option(TEIDE_TEST_VERBOSE "Enable verbose logging for Teide tests" OFF)
option(TEIDE_UNIT_TEST_SW_RENDER "Enable software rendering for Teide unit tests" OFF)
option(TEIDE_RENDER_TEST_SW_RENDER "Enable software rendering for Teide render tests" ON)
option(TEIDE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)

enable_testing()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/modules)

find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(function2 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(RenderDoc)

if(TEIDE_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    find_package(SwiftShader REQUIRED)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if(TEIDE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(TEIDE_BUILD_EXAMPLES)
    add_subdirectory(examples)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)
endif()

if(TEIDE_CLANG_TIDY)
    include(tools/cmake/ClangTidy.cmake)
    td_add_clang_tidy()
endif()
