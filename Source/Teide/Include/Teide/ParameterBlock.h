
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"

#include <vulkan/vulkan.hpp>

#include <vector>

struct ParameterBlockData
{
	vk::DescriptorSetLayout layout;
	std::vector<std::byte> uniformBufferData;
	std::vector<const Texture*> textures;
};

class ParameterBlock
{
public:
	virtual ~ParameterBlock() = default;

	virtual std::size_t GetUniformBufferSize() const = 0;
};
