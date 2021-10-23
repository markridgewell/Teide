
#pragma once

#include <string_view>
#include <vector>

enum class ShaderStage
{
	Vertex,
	Pixel
};

enum class ShaderLanguage
{
	Glsl,
	Hlsl
};

std::vector<uint32_t> CompileShader(std::string_view sourceSource, ShaderStage stage, ShaderLanguage language);
