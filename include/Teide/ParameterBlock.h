
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Shader.h"

#include <vector>

namespace Teide
{

struct ShaderParameters
{
    std::vector<byte> uniformData;
    std::vector<TexturePtr> textures;
};

class ParameterBlockLayout : AbstractBase
{
    virtual bool IsEmpty() const = 0;
};

struct ParameterBlockData
{
    ParameterBlockLayoutPtr layout = nullptr;
    ResourceLifetime lifetime = ResourceLifetime::Permanent;
    ShaderParameters parameters;
};

class ParameterBlock : AbstractBase
{
public:
    virtual usize GetUniformBufferSize() const = 0;
    virtual usize GetPushConstantSize() const = 0;
};

} // namespace Teide
