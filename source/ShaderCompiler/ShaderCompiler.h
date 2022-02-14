
#pragma once

#include "Framework/Shader.h"

#include <exception>
#include <string>
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

class CompileError : public std::exception
{
public:
	explicit CompileError(std::string message) : m_message{std::move(message)} {}

	const char* what() const noexcept override { return m_message.c_str(); }

private:
	std::string m_message;
};

ShaderData CompileShader(std::string_view vertexSource, std::string_view pixelSource, ShaderLanguage language);
