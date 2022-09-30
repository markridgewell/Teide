
#pragma once

#include "Teide/ForwardDeclare.h"

class ShaderEnvironment
{
public:
	virtual ~ShaderEnvironment() = default;

	virtual ParameterBlockLayoutPtr GetScenePblockLayout() const = 0;
	virtual ParameterBlockLayoutPtr GetViewPblockLayout() const = 0;
};
