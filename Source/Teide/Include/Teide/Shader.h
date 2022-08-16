
#pragma once

#include <vulkan/vulkan.hpp>

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
};
