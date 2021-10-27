
#pragma once

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

	const char* what() const override { return m_message.c_str(); }

private:
	std::string m_message;
};

std::vector<uint32_t> CompileShader(std::string_view sourceSource, ShaderStage stage, ShaderLanguage language);
