
#include "Teide/ShaderData.h"

#include "Teide/Definitions.h"

#include <ostream>

namespace Teide
{

namespace
{
    constexpr uint32 MaxPushConstantSize = 128u;

    constexpr auto RoundUp(std::integral auto a, std::integral auto b)
    {
        return ((a - 1) / b + 1) * b;
    }
} // namespace

bool IsResourceType(ShaderVariableType::BaseType type)
{
    using enum ShaderVariableType::BaseType;
    switch (type)
    {
        case Float: return false;
        case Vector2: return false;
        case Vector3: return false;
        case Vector4: return false;
        case Matrix4: return false;

        case Texture2D: return true;
        case Texture2DShadow: return true;
    }
    Unreachable();
}

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType::BaseType type)
{
    using enum ShaderVariableType::BaseType;
    switch (type)
    {
        case Float: return {.size = sizeof(float), .alignment = sizeof(float)};

        case Vector2: return {.size = sizeof(float) * 2, .alignment = sizeof(float) * 2};
        case Vector3: [[fallthrough]]; // Vector3s are padded to 16 bytes
        case Vector4: return {.size = sizeof(float) * 4, .alignment = sizeof(float) * 4};
        case Matrix4: return {.size = sizeof(float) * 4 * 4, .alignment = sizeof(float) * 4};

        case Texture2D: return {};
        case Texture2DShadow: return {};
    }
    Unreachable();
}

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType type)
{
    if (type.arraySize != 0 && type.baseType == ShaderVariableType::BaseType::Vector3)
    {
        const uint32 arraySize = sizeof(float) * 3 * type.arraySize;
        return {.size = arraySize, .alignment = arraySize};
    }
    const auto [size, alignment] = GetSizeAndAlignment(type.baseType);
    return {size * std::max(1u, type.arraySize), alignment};
}

std::string_view ToString(ShaderVariableType::BaseType type)
{
    switch (type)
    {
        using enum ShaderVariableType::BaseType;
        case Float: return "float";
        case Vector2: return "vec2";
        case Vector3: return "vec3";
        case Vector4: return "vec4";
        case Matrix4: return "mat4";
        case Texture2D: return "sampler2D";
        case Texture2DShadow: return "sampler2DShadow";
    }
    Unreachable();
}

std::ostream& operator<<(std::ostream& os, ShaderVariableType::BaseType type)
{
    return os << ToString(type);
}

std::ostream& operator<<(std::ostream& os, ShaderVariableType type)
{
    os << type.baseType;
    if (type.arraySize != 0)
    {
        os << '[' << type.arraySize << ']';
    }
    return os;
}

ParameterBlockLayoutData BuildParameterBlockLayout(const ParameterBlockDesc& pblock, int set)
{
    ParameterBlockLayoutData bindings;
    bindings.uniformsStages = pblock.uniformsStages;

    for (const auto& parameter : pblock.parameters)
    {
        if (IsResourceType(parameter.type.baseType))
        {
            bindings.textureCount++;
        }
        else
        {
            const auto [size, alignment] = GetSizeAndAlignment(parameter.type);
            const auto offset = RoundUp(bindings.uniformsSize, alignment);
            bindings.uniformDescs.push_back(UniformDesc{.name = parameter.name, .type = parameter.type, .offset = offset});
            bindings.uniformsSize = offset + size;
        }
    }

    if (set == 3 && bindings.uniformsSize <= MaxPushConstantSize)
    {
        bindings.isPushConstant = true;
    }

    return bindings;
}

} // namespace Teide
