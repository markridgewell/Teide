cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(CMakeDependentOption)
include(CMakePrintHelpers)
include(tools/cmake/Build.cmake)
include(tools/cmake/Cache.cmake)
include(tools/cmake/CompileCommands.cmake)
include(tools/cmake/Option.cmake)

option_feature(TEIDE_BUILD_EXAMPLES examples "Build examples for Teide" ON)
option_feature(TEIDE_BUILD_TESTS tests "Build tests for Teide" ON)
option_feature(TEIDE_BUILD_BENCHMARKS benchmarks "Build benchmarks for Teide" ON)
option_feature(TEIDE_USE_SWIFTSHADER swiftshader "Enable software rendering using the Swiftshader library" ON)
option(TEIDE_USE_VCPKG "Vendor dependencies through vcpkg" ON)
option(TEIDE_TEST_VERBOSE "Enable verbose logging for Teide tests" OFF)
option(TEIDE_TEST_COVERAGE "Enable code coverage reporting when running tests" OFF)
option(TEIDE_TEST_WINDOWLESS "Disable tests requiring a windowing system" OFF)
option(TEIDE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)
option(TEIDE_CPPCHECK "Enable static analysis with Cppcheck" OFF)
option(TEIDE_CCACHE "Use ccache to accelerate builds" OFF)
option(TEIDE_PRECOMPILED_HEADERS "Enable precompiled headers when building" OFF)
option_enum(TEIDE_SANITIZER "Name of a sanitizer to enable, or OFF to disable" "OFF;ASAN;UBSAN")

cmake_dependent_option(
    TEIDE_UNIT_TEST_SW_RENDER
    "Enable software rendering for Teide unit tests"
    OFF
    TEIDE_USE_SWIFTSHADER
    OFF)
cmake_dependent_option(
    TEIDE_RENDER_TEST_SW_RENDER
    "Enable software rendering for Teide render tests"
    ON
    TEIDE_USE_SWIFTSHADER
    OFF)

if(TEIDE_USE_VCPKG)
    if(DEFINED VCPKG_TARGET_TRIPLET)
        message(STATUS "Using pre-defined vcpkg target triplet ${VCPKG_TARGET_TRIPLET}")
    else()
        include(tools/cmake/Triplet.cmake)
        message(STATUS "Target triplet for vcpkg determined to be ${VCPKG_TARGET_TRIPLET}")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

if(TEIDE_CCACHE)
    include(tools/cmake/Ccache.cmake)
    td_enable_ccache()
endif()

message("Before project: MSVC=${MSVC}, MSVC_VERSION=${MSVC_VERSION}, CMAKE_GENERATOR=${CMAKE_GENERATOR}")

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

message("After project: MSVC=${MSVC}, MSVC_VERSION=${MSVC_VERSION}, CMAKE_GENERATOR=${CMAKE_GENERATOR}")

enable_testing()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/modules)
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/scripts)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(glslang REQUIRED)
find_package(Taskflow REQUIRED)
find_package(function2 REQUIRED)
find_package(stdexec REQUIRED)
find_package(RenderDoc)

if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image ALIAS SDL2_image::SDL2_image-static)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate vkex headers
set(GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/GENERATED/include")
find_package(Python REQUIRED)
set(VK_XML_PATH "${Vulkan_INCLUDE_DIR}/../share/vulkan/registry/vk.xml")
if("${VK_XML_PATH}" IS_NEWER_THAN "${GENERATED_INCLUDE_DIR}/vkex/vkex.hpp")
    execute_process(COMMAND "${Python_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/vkex/genvkex.py" -registry
                            "${VK_XML_PATH}" -o "${GENERATED_INCLUDE_DIR}/vkex" COMMAND_ERROR_IS_FATAL ANY)
endif()

include(FileLists.cmake)
td_add_library(
    Teide
    SOURCES "${sources}" "${GENERATED_INCLUDE_DIR}/vkex/vkex.hpp"
    PUBLIC_DEPS spdlog::spdlog
    PRIVATE_DEPS
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        SDL2::SDL2
        fmt::fmt
        Taskflow::Taskflow
        function2::function2
        STDEXEC::stdexec
        glslang::SPIRV
        glslang::glslang)
target_include_directories(Teide SYSTEM PRIVATE "${GENERATED_INCLUDE_DIR}")

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
    find_package(assimp REQUIRED)

    add_subdirectory(examples)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)
endif()

if(TEIDE_USE_SWIFTSHADER)
    find_package(SwiftShader REQUIRED)
endif()

if(TEIDE_BUILD_TESTS)
    find_package(GTest REQUIRED)
    find_package(StackWalker QUIET)

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

dump_targets()
