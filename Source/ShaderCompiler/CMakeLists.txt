td_add_library(ShaderCompiler SOURCES Include/ShaderCompiler/ShaderCompiler.h Source/ShaderCompiler.cpp)

target_link_libraries(
    ShaderCompiler
    PUBLIC Renderer
           HLSL
           SPIRV
           glslang
           OGLCompiler)
