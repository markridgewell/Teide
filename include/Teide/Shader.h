
#pragma once

#include "Teide/ForwardDeclare.h"

namespace Teide
{

enum class ParameterBlockType
{
    Scene,
    View,
    Material,
    Object,
};

class Shader
{
public:
    virtual ~Shader() = default;

    virtual ParameterBlockLayoutPtr GetMaterialPblockLayout() const = 0;
    virtual ParameterBlockLayoutPtr GetObjectPblockLayout() const = 0;
};

} // namespace Teide
