
#pragma once

#include "Types/ShaderData.h"

#include <exception>
#include <string>
#include <string_view>
#include <vector>

enum class ShaderLanguage
{
	Glsl,
	Hlsl,
};

enum class UniformBaseType
{
	Float,
	Int,
	Uint,
};

struct UniformType
{
	UniformBaseType baseType;
	std::uint8_t rowCount = 1;
	std::uint8_t columnCount = 1;
};

struct UniformDefinition
{
	std::string name;
	UniformType type;
	std::size_t arraySize = 0;
};

struct VaryingDefinition
{
	std::string name;
	UniformType type;
};

struct TextureBindingDefinition
{
	std::string name;
};

struct ParameterBlockDefinition
{
	std::vector<UniformDefinition> uniforms;
	std::vector<TextureBindingDefinition> textures;
};

struct ShaderStageDefinition
{
	std::vector<VaryingDefinition> inputs;
	std::vector<VaryingDefinition> outputs;
	std::string source;
};

struct ShaderSourceData
{
	ShaderLanguage language;
	ParameterBlockDefinition scenePblock;
	ParameterBlockDefinition viewPblock;
	ParameterBlockDefinition materialPblock;
	ParameterBlockDefinition objectPblock;
	ShaderStageDefinition vertexShader;
	ShaderStageDefinition pixelShader;
};

class CompileError : public std::exception
{
public:
	explicit CompileError(std::string message) : m_message{std::move(message)} {}

	const char* what() const noexcept override { return m_message.c_str(); }

private:
	std::string m_message;
};

ShaderData CompileShader(std::string_view vertexSource, std::string_view pixelSource, ShaderLanguage language);
ShaderData CompileShader(const ShaderSourceData& sourceData);
