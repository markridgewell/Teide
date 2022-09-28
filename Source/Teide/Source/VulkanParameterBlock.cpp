
#include "VulkanParameterBlock.h"

#include "Teide/Buffer.h"

VulkanParameterBlock::VulkanParameterBlock(const VulkanParameterBlockLayout& layout)
{
	if (layout.pushConstantRange.has_value())
	{
		pushConstantData.resize(layout.pushConstantRange->size - layout.pushConstantRange->offset);
	}
}

std::size_t VulkanParameterBlock::GetUniformBufferSize() const
{
	return uniformBuffer ? uniformBuffer->GetSize() : 0;
}

std::size_t VulkanParameterBlock::GetPushConstantSize() const
{
	return pushConstantData.size();
}
