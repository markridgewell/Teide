cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(CMakeDependentOption)
include(tools/cmake/Build.cmake)
include(tools/cmake/Cache.cmake)
include(tools/cmake/CompileCommands.cmake)

if(LINUX AND TEIDE_SANITIZER STREQUAL "UBSAN")
    set(VCPKG_TARGET_TRIPLET "x64-linux-ubsan")
elseif(
    LINUX
    AND CMAKE_CXX_COMPILER MATCHES "clang"
    AND TEIDE_SANITIZER STREQUAL "MSAN")
    #find_package(libcxx REQUIRED)
    set(VCPKG_TARGET_TRIPLET "x64-linux-msan")
endif()

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

option(TEIDE_BUILD_EXAMPLES "Build examples for Teide" ON)
option(TEIDE_BUILD_TESTS "Build tests for Teide" ON)
option(TEIDE_BUILD_BENCHMARKS "Build benchmarks for Teide" ON)
option(TEIDE_TEST_VERBOSE "Enable verbose logging for Teide tests" OFF)
option(TEIDE_TEST_COVERAGE "Enable code coverage reporting when running tests" OFF)
option(TEIDE_TEST_WINDOWLESS "Disable tests requiring a windowing system" OFF)
option(TEIDE_UNIT_TEST_SW_RENDER "Enable software rendering for Teide unit tests" OFF)
option(TEIDE_RENDER_TEST_SW_RENDER "Enable software rendering for Teide render tests" ON)
option(TEIDE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)
option(TEIDE_CPPCHECK "Enable static analysis with Cppcheck" OFF)
option(TEIDE_PRECOMPILED_HEADERS "Enable precompiled headers when building" OFF)

set(TEIDE_SANITIZER
    OFF
    CACHE STRING "Name of a sanitizer to enable, or OFF to disable")
set_property(CACHE TEIDE_SANITIZER PROPERTY STRINGS "OFF;ASAN;UBSAN;MSAN")

enable_testing()

set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/scripts)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(glslang REQUIRED)
find_package(assimp REQUIRED)
find_package(Taskflow REQUIRED)
find_package(function2 REQUIRED)
find_package(RenderDoc)

if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image ALIAS SDL2_image::SDL2_image-static)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FileLists.cmake)
td_add_library(
    Teide
    SOURCES "${sources}"
    PUBLIC_DEPS spdlog::spdlog
    PRIVATE_DEPS
        Vulkan::Vulkan
        VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        SDL2::SDL2
        fmt::fmt
        Taskflow::Taskflow
        function2::function2
        glslang::SPIRV
        glslang::glslang)

if(TEIDE_PRECOMPILED_HEADERS)
    target_precompile_headers(
        Teide
        PRIVATE
        src/Teide/VulkanConfig.h
        <algorithm>
        <ranges>
        <string>
        <vector>)
    set_source_files_properties(src/Teide/VmaImpl.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
endif()

if(TEIDE_BUILD_EXAMPLES)
    add_subdirectory(examples)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)
endif()

if(TEIDE_BUILD_TESTS)
    find_package(GTest REQUIRED)
    find_package(SwiftShader REQUIRED)

    add_subdirectory(tests)
endif()

if(TEIDE_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)

    add_subdirectory(benchmarks)
endif()

if(TEIDE_CLANG_TIDY)
    include(tools/cmake/ClangTidy.cmake)
    td_add_clang_tidy()
endif()

if(TEIDE_CPPCHECK)
    include(tools/cmake/Cppcheck.cmake)
    td_add_cppcheck()
endif()

mark_as_advanced_pattern("[a-z0-9]+_DIR|VCPKG_|FETCHCONTENT_")

dump_targets("${CMAKE_BINARY_DIR}/alltargets.txt")
