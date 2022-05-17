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
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Application)

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

# ----- Framework -----

set(sources
    source/Framework/Buffer.h
    source/Framework/Buffer.cpp
    source/Framework/BytesView.h
    source/Framework/ForwardDeclare.h
    source/Framework/GraphicsDevice.h
    source/Framework/GraphicsDevice.cpp
    source/Framework/ParameterBlock.h
    source/Framework/ParameterBlock.cpp
    source/Framework/Pipeline.h
    source/Framework/Renderer.h
    source/Framework/Renderer.cpp
    source/Framework/Shader.h
    source/Framework/Surface.h
    source/Framework/Surface.cpp
    source/Framework/Texture.h
    source/Framework/Texture.cpp
    source/Framework/Internal/CommandBuffer.h
    source/Framework/Internal/CommandBuffer.cpp
    source/Framework/Internal/CpuExecutor.h
    source/Framework/Internal/CpuExecutor.cpp
    source/Framework/Internal/GpuExecutor.h
    source/Framework/Internal/GpuExecutor.cpp
    source/Framework/Internal/MemoryAllocator.h
    source/Framework/Internal/MemoryAllocator.cpp
    source/Framework/Internal/Vulkan.h
    source/Framework/Internal/Vulkan.cpp
    Vulkan.natvis)

add_library(Framework ${sources})

target_link_libraries(
    Framework
    PUBLIC Vulkan::Vulkan
           SDL2::SDL2
           fmt::fmt
           spdlog::spdlog
           Taskflow::Taskflow)

target_compile_definitions(Framework PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_compile_features(Framework PRIVATE ${cxx_standard})
target_include_directories(Framework PUBLIC source)

# ----- ShaderCompiler -----

set(sources source/ShaderCompiler/ShaderCompiler.h source/ShaderCompiler/ShaderCompiler.cpp)

add_library(ShaderCompiler ${sources})

target_link_libraries(
    ShaderCompiler
    PUBLIC Framework
           HLSL
           SPIRV
           glslang
           OGLCompiler)

target_compile_features(ShaderCompiler PRIVATE ${cxx_standard})
target_include_directories(ShaderCompiler PUBLIC source/ShaderCompiler)

# ----- FrameworkTest -----

set(sources
    test/Framework/Main.cpp
    test/Framework/GraphicsDeviceTest.cpp
    test/Framework/CpuExecutorTest.cpp
    test/Framework/GpuExecutorTest.cpp
    test/Framework/RendererTest.cpp
    test/Framework/SurfaceTest.cpp)

add_executable(FrameworkTest ${sources})
target_compile_features(FrameworkTest PRIVATE ${cxx_standard})
target_link_libraries(
    FrameworkTest
    PRIVATE Framework
            ShaderCompiler
            GTest::gtest
            GTest::gmock
            fmt::fmt)
add_test(NAME FrameworkTest COMMAND FrameworkTest)

# ----- Application -----

set(sources source/Application.cpp source/Definitions.h)

add_executable(Application WIN32 ${sources})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
target_compile_features(Application PRIVATE ${cxx_standard})
set_property(TARGET Application PROPERTY VS_DPI_AWARE "PerMonitor")

target_link_libraries(
    Application
    PRIVATE GeoLib
            Framework
            ShaderCompiler
            SDL2::SDL2
            SDL2::SDL2main
            fmt::fmt
            spdlog::spdlog
            assimp::assimp
            Taskflow::Taskflow)
target_include_directories(Application PRIVATE ${STB_INCLUDE_DIRS})

# Visual Studio debugging environment, from preset
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND ${TEIDE_VS_DEBUGGER_COMMAND})
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS ${TEIDE_VS_DEBUGGER_COMMAND_ARGUMENTS})
set_property(TARGET Application PROPERTY VS_DEBUGGER_ENVIRONMENT ${TEIDE_VS_DEBUGGER_ENVIRONMENT})
set_property(TARGET Application PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${TEIDE_VS_DEBUGGER_WORKING_DIRECTORY})
