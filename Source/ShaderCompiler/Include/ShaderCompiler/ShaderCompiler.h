
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

struct UniformDefinition
{
	std::string name;
	ShaderVariableType type;
};

struct VaryingDefinition
{
	std::string name;
	ShaderVariableType type;
};

enum class ResourceType
{
	Texture2D,
	Texture2DShadow,
};

struct ResourceBindingDefinition
{
	std::string name;
	ResourceType type;
};

struct ParameterBlockDefinition
{
	std::vector<UniformDefinition> uniforms;
	std::vector<ResourceBindingDefinition> resources;
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

ShaderData CompileShader(const ShaderSourceData& sourceData);
