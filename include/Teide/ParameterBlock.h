
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Shader.h"

#include <vector>

namespace Teide
{

struct ShaderParameters
{
	std::vector<std::byte> uniformData;
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

	virtual std::size_t GetUniformBufferSize() const = 0;
	virtual std::size_t GetPushConstantSize() const = 0;
};

} // namespace Teide
