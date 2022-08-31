
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

struct ShaderVariable
{
	std::string name;
	ShaderVariableType type;
};

struct VaryingDefinition
{
	std::string name;
	ShaderVariableType type;
};

struct ParameterBlockDesc
{
	std::vector<ShaderVariable> parameters;
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
	ParameterBlockDesc scenePblock;
	ParameterBlockDesc viewPblock;
	ParameterBlockDesc materialPblock;
	ParameterBlockDesc objectPblock;
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
