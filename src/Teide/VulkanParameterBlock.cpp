
#include "VulkanParameterBlock.h"

#include "Teide/Buffer.h"

namespace Teide
{

bool VulkanParameterBlockLayout::IsEmpty() const
{
    return !(HasDescriptors() || HasPushConstants());
}

bool VulkanParameterBlockLayout::HasDescriptors() const
{
    return !descriptorTypeCounts.empty();
}

bool VulkanParameterBlockLayout::HasPushConstants() const
{
    return pushConstantRange.has_value();
}

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
