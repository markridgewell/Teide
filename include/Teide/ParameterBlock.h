
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Handle.h"
#include "Teide/Shader.h"

#include <vector>

namespace Teide
{

struct ShaderParameters
{
    std::vector<byte> uniformData;
    std::vector<Texture> textures;
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

class ParameterBlock : public Handle<>
{
public:
    using Handle::Handle;

    bool operator==(const ParameterBlock&) const = default;
};

} // namespace Teide
