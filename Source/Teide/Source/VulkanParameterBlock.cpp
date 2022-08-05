
#include "VulkanParameterBlock.h"

#include "Teide/Buffer.h"

std::size_t VulkanParameterBlock::GetUniformBufferSize() const
{
	return uniformBuffer->GetSize();
}

void VulkanParameterBlock::SetUniformData(int currentFrame, BytesView data)
{
	uniformBuffer->SetData(currentFrame, data);
}
