
#pragma once

#include <cstdint>
#include <vector>

enum class ShaderStageFlags
{
	Vertex = 1 << 0,
	Pixel = 1 << 1,
};

constexpr ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b)
{
	return ShaderStageFlags{static_cast<int>(a) | static_cast<int>(b)};
}
constexpr ShaderStageFlags& operator|=(ShaderStageFlags& a, ShaderStageFlags b)
{
	return a = a | b;
}
constexpr ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b)
{
	return ShaderStageFlags{static_cast<int>(a) & static_cast<int>(b)};
}

struct ParameterBlockLayout
{
	std::uint32_t uniformsSize = 0;
	std::uint32_t textureCount = 0;
	bool isPushConstant = false;
	ShaderStageFlags uniformsStages = {};
};

struct ShaderData
{
	ParameterBlockLayout sceneBindings;
	ParameterBlockLayout viewBindings;
	ParameterBlockLayout materialBindings;
	ParameterBlockLayout objectBindings;
	std::vector<std::uint32_t> vertexShaderSpirv;
	std::vector<std::uint32_t> pixelShaderSpirv;
};
