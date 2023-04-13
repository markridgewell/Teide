
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/ForwardDeclare.h"

namespace Teide
{

class ShaderEnvironment : AbstractBase
{
public:
    virtual ParameterBlockLayoutPtr GetScenePblockLayout() const = 0;
    virtual ParameterBlockLayoutPtr GetViewPblockLayout() const = 0;
};

} // namespace Teide
