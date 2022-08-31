
#pragma once

#include <cstdint>
#include <string>
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

struct ShaderVariableType
{
	enum class BaseType
	{
		// Uniform types
		Float,
		Vector2,
		Vector3,
		Vector4,
		Matrix4,

		// Resource types
		Texture2D,
		Texture2DShadow,
	};

	ShaderVariableType(BaseType baseType, std::uint32_t arraySize = 0) : baseType{baseType}, arraySize{arraySize} {}

	BaseType baseType;
	std::uint32_t arraySize = 0;
};

struct UniformDesc
{
	std::string name;
	ShaderVariableType type;
	std::uint32_t offset = 0;
};

struct ParameterBlockLayout
{
	std::uint32_t uniformsSize = 0;
	std::vector<UniformDesc> uniformDescs;
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
