
#pragma once

#include "Teide/ForwardDeclare.h"

namespace Teide
{

class ShaderEnvironment
{
public:
    virtual ~ShaderEnvironment() = default;

    virtual ParameterBlockLayoutPtr GetScenePblockLayout() const = 0;
    virtual ParameterBlockLayoutPtr GetViewPblockLayout() const = 0;
};

} // namespace Teide
