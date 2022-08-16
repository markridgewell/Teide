
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Shader.h"

#include <vector>

struct ShaderParameters
{
	std::vector<std::byte> uniformBufferData;
	std::vector<const Texture*> textures;
};

struct ParameterBlockData
{
	ShaderPtr shader;
	ParameterBlockType blockType;
	ShaderParameters parameters;
};

class ParameterBlock
{
public:
	virtual ~ParameterBlock() = default;

	virtual std::size_t GetUniformBufferSize() const = 0;
};
