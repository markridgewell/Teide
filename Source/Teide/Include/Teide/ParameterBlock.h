
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"

#include <vulkan/vulkan.hpp>

#include <vector>

struct ParameterBlockData
{
	vk::DescriptorSetLayout layout;
	vk::DeviceSize uniformBufferSize = 0;
	std::vector<const Texture*> textures;
};

struct ParameterBlock
{
	DynamicBufferPtr uniformBuffer;
	std::vector<vk::UniqueDescriptorSet> descriptorSet;

	void SetUniformData(int currentFrame, BytesView data);
};
