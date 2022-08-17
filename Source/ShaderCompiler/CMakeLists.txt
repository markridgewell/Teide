td_add_library(ShaderCompiler SOURCES Include/ShaderCompiler/ShaderCompiler.h Source/ShaderCompiler.cpp)

target_link_libraries(
    ShaderCompiler
    PUBLIC Types
           HLSL
           SPIRV
           glslang
           OGLCompiler
           fmt::fmt)

# Test
td_add_test(ShaderCompilerTest SOURCES Test/ShaderCompilerTest.cpp Test/Main.cpp)

target_link_libraries(ShaderCompilerTest PRIVATE ShaderCompiler GTest::gtest GTest::gmock)
