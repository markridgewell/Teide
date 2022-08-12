
#pragma once

#include "Teide/ParameterBlock.h"
#include "Vulkan.h"

struct VulkanParameterBlock : public ParameterBlock
{
	BufferPtr uniformBuffer;
	vk::UniqueDescriptorSet descriptorSet;

	std::size_t GetUniformBufferSize() const override;
};

template <>
struct VulkanImpl<ParameterBlock>
{
	using type = VulkanParameterBlock;
};
