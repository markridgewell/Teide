
#pragma once

#include "Teide/BasicTypes.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace Teide
{

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

    ShaderVariableType(BaseType baseType, uint32 arraySize = 0) : // cppcheck-suppress noExplicitConstructor
        baseType{baseType}, arraySize{arraySize}
    {}

    BaseType baseType;
    uint32 arraySize = 0;

    bool operator==(const ShaderVariableType&) const = default;
};

struct ShaderVariable
{
    std::string name;
    ShaderVariableType type;

    bool operator==(const ShaderVariable&) const = default;
};

struct ParameterBlockDesc
{
    std::vector<ShaderVariable> parameters;
    ShaderStageFlags uniformsStages = {};

    bool operator==(const ParameterBlockDesc&) const = default;
};

struct ShaderEnvironmentData
{
    ParameterBlockDesc scenePblock;
    ParameterBlockDesc viewPblock;

    bool operator==(const ShaderEnvironmentData&) const = default;
};

struct ShaderStageData
{
    std::vector<ShaderVariable> inputs;
    std::vector<ShaderVariable> outputs;
    std::vector<uint32> spirv;

    bool operator==(const ShaderStageData&) const = default;
};

struct ShaderData
{
    ShaderEnvironmentData environment;
    ParameterBlockDesc materialPblock;
    ParameterBlockDesc objectPblock;
    ShaderStageData vertexShader;
    ShaderStageData pixelShader;

    bool operator==(const ShaderData&) const = default;
};

bool IsResourceType(ShaderVariableType::BaseType type);

struct SizeAndAlignment
{
    uint32 size = 0;
    uint32 alignment = 0;
};

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType::BaseType type);
SizeAndAlignment GetSizeAndAlignment(ShaderVariableType type);

std::ostream& operator<<(std::ostream& os, ShaderVariableType::BaseType type);
std::ostream& operator<<(std::ostream& os, ShaderVariableType type);

struct UniformDesc
{
    std::string name;
    ShaderVariableType type;
    uint32 offset = 0;

    bool operator==(const UniformDesc&) const = default;
    void Visit(auto f) const { f(name, type, offset); }
};

struct ParameterBlockLayoutData
{
    uint32 uniformsSize = 0;
    std::vector<UniformDesc> uniformDescs;
    uint32 textureCount = 0;
    bool isPushConstant = false;
    ShaderStageFlags uniformsStages = {};
};

ParameterBlockLayoutData BuildParameterBlockLayout(const ParameterBlockDesc& pblock, int set);

} // namespace Teide
