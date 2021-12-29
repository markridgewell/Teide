cmake_minimum_required(VERSION 3.19)

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
find_path(STB_INCLUDE_DIRS "stb.h")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Teide)

if(MSVC)
    # Current Visual Studio versions require /std:c++latest for some C++20 features, which requires cxx_std_23
    set(cxx_standard cxx_std_23)
else()
    set(cxx_standard cxx_std_20)
endif()

include(CTest)
enable_testing()

# ----- GeoLib -----

set(GeoLibInclude source)
set(sources
    ${GeoLibInclude}/GeoLib/ForwardDeclare.h
    ${GeoLibInclude}/GeoLib/Vector.h
    ${GeoLibInclude}/GeoLib/Matrix.h
    ${GeoLibInclude}/GeoLib/Angle.h
    ${GeoLibInclude}/GeoLib/Scalar.h
    GeoLib.natvis)

add_library(GeoLib INTERFACE ${sources})
target_include_directories(GeoLib INTERFACE ${GeoLibInclude})
target_compile_features(GeoLib INTERFACE ${cxx_standard})

# ----- GeoLibTest -----

set(sources
    test/GeoLib/Main.cpp
    test/GeoLib/TestUtils.h
    test/GeoLib/AngleTest.cpp
    test/GeoLib/MatrixTest.cpp
    test/GeoLib/VectorTest.cpp)

add_executable(GeoLibTest ${sources})
target_compile_features(GeoLibTest PRIVATE ${cxx_standard})
target_link_libraries(GeoLibTest PRIVATE GeoLib GTest::gtest GTest::gmock fmt::fmt)
add_test(NAME GeoLibTest COMMAND GeoLibTest)

# ----- Application -----

set(sources
    source/Application.cpp
    source/ShaderCompiler.h
    source/ShaderCompiler.cpp
    source/Definitions.h
    source/Framework/Buffer.h
    source/Framework/Buffer.cpp
    source/Framework/BytesView.h
    source/Framework/MemoryAllocator.h
    source/Framework/MemoryAllocator.cpp
    source/Framework/Renderer.h
    source/Framework/Renderer.cpp
    source/Framework/Surface.h
    source/Framework/Surface.cpp
    source/Framework/Texture.h
    source/Framework/Texture.cpp
    source/Framework/Vulkan.h
    source/Framework/Vulkan.cpp
    Vulkan.natvis)

add_executable(Teide WIN32 ${sources})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
target_compile_features(Teide PRIVATE ${cxx_standard})
target_compile_definitions(Teide PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
set_property(TARGET Teide PROPERTY VS_DPI_AWARE "PerMonitor")

target_link_libraries(
    Teide
    PRIVATE GeoLib
            Vulkan::Vulkan
            SDL2::SDL2
            SDL2::SDL2main
            fmt::fmt
            spdlog::spdlog
            HLSL
            SPIRV
            glslang
            OGLCompiler
            assimp::assimp
            Taskflow::Taskflow)
target_include_directories(Teide PRIVATE ${STB_INCLUDE_DIRS})

# Visual Studio debugging environment, from preset
set_property(TARGET Teide PROPERTY VS_DEBUGGER_COMMAND ${TEIDE_VS_DEBUGGER_COMMAND})
set_property(TARGET Teide PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS ${TEIDE_VS_DEBUGGER_COMMAND_ARGUMENTS})
set_property(TARGET Teide PROPERTY VS_DEBUGGER_ENVIRONMENT ${TEIDE_VS_DEBUGGER_ENVIRONMENT})
set_property(TARGET Teide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${TEIDE_VS_DEBUGGER_WORKING_DIRECTORY})
