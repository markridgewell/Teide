
#pragma once

#include "Teide/ParameterBlock.h"
#include "Vulkan.h"

#include <memory>
#include <optional>
#include <vector>

namespace Teide
{

struct VulkanParameterBlockLayout : public ParameterBlockLayout
{
	vk::UniqueDescriptorSetLayout setLayout;
	std::optional<vk::PushConstantRange> pushConstantRange;
	vk::ShaderStageFlags uniformsStages;
};

using VulkanParameterBlockLayoutPtr = std::shared_ptr<const VulkanParameterBlockLayout>;

template <>
struct VulkanImpl<ParameterBlockLayout>
{
	using type = VulkanParameterBlockLayout;
};

struct VulkanParameterBlock : public ParameterBlock
{
	BufferPtr uniformBuffer;
	vk::UniqueDescriptorSet descriptorSet;
	std::vector<std::byte> pushConstantData;

	explicit VulkanParameterBlock(const VulkanParameterBlockLayout& layout);

	std::size_t GetUniformBufferSize() const override;
	std::size_t GetPushConstantSize() const override;
};

template <>
struct VulkanImpl<ParameterBlock>
{
	using type = VulkanParameterBlock;
};

} // namespace Teide
