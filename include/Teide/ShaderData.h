
#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <vector>

enum class ShaderStageFlags
{
	None = 0,
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

	auto operator<=>(const ShaderVariableType&) const = default;
};

struct ShaderVariable
{
	std::string name;
	ShaderVariableType type;

	auto operator<=>(const ShaderVariable&) const = default;
};

struct ParameterBlockDesc
{
	std::vector<ShaderVariable> parameters;
	ShaderStageFlags uniformsStages = {};

	auto operator<=>(const ParameterBlockDesc&) const = default;
};

struct ShaderEnvironmentData
{
	ParameterBlockDesc scenePblock;
	ParameterBlockDesc viewPblock;
};

struct ShaderStageData
{
	std::vector<ShaderVariable> inputs;
	std::vector<ShaderVariable> outputs;
	std::vector<std::uint32_t> spirv;
};

struct ShaderData
{
	ShaderEnvironmentData environment;
	ParameterBlockDesc materialPblock;
	ParameterBlockDesc objectPblock;
	ShaderStageData vertexShader;
	ShaderStageData pixelShader;

	auto operator<=>(const ShaderData&) const = default;
};

bool IsResourceType(ShaderVariableType::BaseType type);

struct SizeAndAlignment
{
	std::uint32_t size = 0;
	std::uint32_t alignment = 0;
};

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType::BaseType type);
SizeAndAlignment GetSizeAndAlignment(ShaderVariableType type);

std::string ToString(ShaderVariableType::BaseType type);
std::string ToString(ShaderVariableType type);

struct UniformDesc
{
	std::string name;
	ShaderVariableType type;
	std::uint32_t offset = 0;
};

struct ParameterBlockLayoutData
{
	std::uint32_t uniformsSize = 0;
	std::vector<UniformDesc> uniformDescs;
	std::uint32_t textureCount = 0;
	bool isPushConstant = false;
	ShaderStageFlags uniformsStages = {};
};

ParameterBlockLayoutData BuildParameterBlockLayout(const ParameterBlockDesc& pblock, int set);
