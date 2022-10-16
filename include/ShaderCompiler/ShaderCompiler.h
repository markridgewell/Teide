
#pragma once

#include "Teide/ShaderData.h"

#include <exception>
#include <string>
#include <string_view>
#include <vector>

enum class ShaderLanguage
{
	Glsl,
	Hlsl,
};

struct ShaderStageDefinition
{
	std::vector<ShaderVariable> inputs;
	std::vector<ShaderVariable> outputs;
	std::string source;
};

struct ShaderSourceData
{
	ShaderLanguage language;
	ShaderEnvironmentData environment;
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
