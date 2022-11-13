
#include "VulkanParameterBlock.h"

#include "Teide/Buffer.h"

namespace Teide
{

VulkanParameterBlock::VulkanParameterBlock(const VulkanParameterBlockLayout& layout)
{
	if (layout.pushConstantRange.has_value())
	{
		pushConstantData.resize(layout.pushConstantRange->size - layout.pushConstantRange->offset);
	}
}

usize VulkanParameterBlock::GetUniformBufferSize() const
{
	return uniformBuffer ? uniformBuffer->GetSize() : 0;
}

usize VulkanParameterBlock::GetPushConstantSize() const
{
	return pushConstantData.size();
}

} // namespace Teide
