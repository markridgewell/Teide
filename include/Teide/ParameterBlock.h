
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Shader.h"

#include <vector>

namespace Teide
{

struct ShaderParameters
{
	std::vector<byte> uniformData;
	std::vector<const Texture*> textures;
};

class ParameterBlockLayout
{
public:
	virtual ~ParameterBlockLayout() = default;
};

struct ParameterBlockData
{
	ParameterBlockLayoutPtr layout = nullptr;
	ResourceLifetime lifetime = ResourceLifetime::Permanent;
	ShaderParameters parameters;
};

class ParameterBlock
{
public:
	virtual ~ParameterBlock() = default;

	virtual usize GetUniformBufferSize() const = 0;
	virtual usize GetPushConstantSize() const = 0;
};

} // namespace Teide
