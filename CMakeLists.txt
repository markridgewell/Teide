cmake_minimum_required(VERSION 3.19)

include(CMakeParseArguments)
include(Build.cmake)

project(
    Teide
    VERSION 0.0.0
    DESCRIPTION "Vulkan-based 3D renderer"
    LANGUAGES CXX)

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
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)

if(MSVC)
    # Current Visual Studio versions require /std:c++latest for some C++20 features, which requires cxx_std_23
    set(cxx_standard cxx_std_23)
else()
    set(cxx_standard cxx_std_20)
endif()

include(CTest)
enable_testing()

add_subdirectory(Source/GeoLib)
add_subdirectory(Source/Teide)
add_subdirectory(Source/Types)
add_subdirectory(Source/ShaderCompiler)
add_subdirectory(Source/Application)
