
#pragma once

#include "Teide/ParameterBlock.h"
#include "Vulkan.h"

struct VulkanParameterBlock : public ParameterBlock
{
	DynamicBufferPtr uniformBuffer;
	std::vector<vk::UniqueDescriptorSet> descriptorSet;

	std::size_t GetUniformBufferSize() const override;
	void SetUniformData(int currentFrame, BytesView data) override;
};

template <>
struct VulkanImpl<ParameterBlock>
{
	using type = VulkanParameterBlock;
};
