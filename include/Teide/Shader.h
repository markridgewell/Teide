
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/ForwardDeclare.h"

namespace Teide
{

enum class ParameterBlockType : uint8
{
    Scene,
    View,
    Material,
    Object,
};

class Shader : AbstractBase
{
public:
    virtual ParameterBlockLayoutPtr GetMaterialPblockLayout() const = 0;
    virtual ParameterBlockLayoutPtr GetObjectPblockLayout() const = 0;
};

} // namespace Teide
