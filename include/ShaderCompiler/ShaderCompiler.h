
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/ShaderData.h"

#include <atomic>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

enum class ShaderLanguage : Teide::uint8
{
    Glsl,
    Hlsl,
};

struct ShaderStageDefinition
{
    std::vector<Teide::ShaderVariable> inputs;
    std::vector<Teide::ShaderVariable> outputs;
    std::string source;
};

struct ShaderSourceData
{
    ShaderLanguage language = ShaderLanguage::Glsl;
    Teide::ShaderEnvironmentData environment;
    Teide::ParameterBlockDesc materialPblock;
    Teide::ParameterBlockDesc objectPblock;
    ShaderStageDefinition vertexShader;
    ShaderStageDefinition pixelShader;
};

struct KernelSourceData
{
    ShaderLanguage language = ShaderLanguage::Glsl;
    Teide::ShaderEnvironmentData environment;
    Teide::ParameterBlockDesc paramsPblock;
    ShaderStageDefinition kernelShader;
};

class CompileError : public std::exception
{
public:
    explicit CompileError(std::string message) : m_message{std::move(message)} {}

    [[nodiscard]] const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

class ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler(ShaderCompiler&&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(ShaderCompiler&&) = delete;

    Teide::ShaderData Compile(const ShaderSourceData& sourceData) const;
    Teide::KernelData Compile(const KernelSourceData& sourceData) const;
};
