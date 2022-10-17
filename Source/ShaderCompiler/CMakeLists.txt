td_add_library(
    ShaderCompiler
    SOURCES Include/ShaderCompiler/ShaderCompiler.h Source/ShaderCompiler.cpp
    PUBLIC_DEPS Types
    PRIVATE_DEPS
        glslang::HLSL
        glslang::SPIRV
        glslang::glslang
        glslang::OGLCompiler
        fmt::fmt)

td_add_test(
    ShaderCompilerTest
    SOURCES Test/ShaderCompilerTest.cpp Test/Main.cpp
    DEPS ShaderCompiler GTest::gtest GTest::gmock)
