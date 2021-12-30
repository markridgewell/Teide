
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Vulkan.h"

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
