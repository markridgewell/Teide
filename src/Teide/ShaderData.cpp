
#include "Teide/ShaderData.h"

#include "Teide/Assert.h"
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
            case RWTexture2D: return "image2D";
        }
        Unreachable();
    }

    void AddUniformBinding(ParameterBlockLayoutData& bindings, const ShaderVariable& var, uint32 size, uint32 alignment)
    {
        const auto offset = RoundUp(bindings.uniformsSize, alignment);
        bindings.uniformDescs.push_back(UniformDesc{.name = var.name, .type = var.type, .offset = offset});
        bindings.uniformsSize = offset + size * std::max(1u, var.type.arraySize);
    }

    void AddResourceBinding(ParameterBlockLayoutData& bindings, const ShaderVariable& var)
    {
        TEIDE_ASSERT(var.type.arraySize <= 1, "Resource arrays not supported yet!");
        bindings.resourceDescs.push_back(var.type.baseType);
    }

} // namespace

ParameterBlockLayoutData BuildParameterBlockLayout(const ParameterBlockDesc& pblock, int set)
{
    ParameterBlockLayoutData bindings;
    bindings.uniformsStages = pblock.uniformsStages;

    for (const auto& parameter : pblock.parameters)
    {
        using enum ShaderVariableType::BaseType;
        switch (parameter.type.baseType)
        {
            case Float: AddUniformBinding(bindings, parameter, sizeof(float), sizeof(float)); break;
            case Vector2: AddUniformBinding(bindings, parameter, sizeof(float) * 2, sizeof(float) * 2); break;
            case Vector3:
                if (parameter.type.arraySize != 0)
                {
                    AddUniformBinding(bindings, parameter, sizeof(float) * 3, sizeof(float) * 3);
                    break;
                }
                [[fallthrough]]; // Vector3s are padded to 16 bytes
            case Vector4: AddUniformBinding(bindings, parameter, sizeof(float) * 4, sizeof(float) * 4); break;
            case Matrix4: AddUniformBinding(bindings, parameter, sizeof(float) * 4 * 4, sizeof(float) * 4); break;

            case Texture2D:
            case Texture2DShadow:
            case RWTexture2D: AddResourceBinding(bindings, parameter); break;
        }
    }

    if (set == 3 && bindings.uniformsSize <= MaxPushConstantSize)
    {
        bindings.isPushConstant = true;
    }

    return bindings;
}

bool IsResourceType(ShaderVariableType::BaseType type)
{
    using enum ShaderVariableType::BaseType;
    switch (type)
    {
        case Float:
        case Vector2:
        case Vector3:
        case Vector4:
        case Matrix4: return false;

        case Texture2D:
        case Texture2DShadow:
        case RWTexture2D: return true;
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


} // namespace Teide
