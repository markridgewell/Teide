
#include "VulkanParameterBlock.h"

#include "Teide/Buffer.h"

std::size_t VulkanParameterBlock::GetUniformBufferSize() const
{
	return uniformBuffer->GetSize();
}
