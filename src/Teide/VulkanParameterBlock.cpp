
#include "VulkanParameterBlock.h"

#include "VulkanConfig.h"

#include "Teide/Buffer.h"
#include "vkex/vkex.hpp"

#include <ranges>

namespace Teide
{
namespace
{
    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    vk::ShaderStageFlags GetShaderStageFlags(ShaderStageFlags flags)
    {
        vk::ShaderStageFlags ret{};
        if ((flags & ShaderStageFlags::Vertex) != ShaderStageFlags{})
        {
            ret |= vk::ShaderStageFlagBits::eVertex;
        }
        if ((flags & ShaderStageFlags::Pixel) != ShaderStageFlags{})
        {
            ret |= vk::ShaderStageFlagBits::eFragment;
        }
        return ret;
    }
} // namespace

VulkanParameterBlockLayout::VulkanParameterBlockLayout(ParameterBlockLayoutData data, vk::Device device)
{
    std::optional<vk::DescriptorSetLayoutBinding> uniformBinding;

    if (data.uniformsSize > 0u)
    {
        if (data.isPushConstant)
        {
            pushConstantRange = {
                .stageFlags = GetShaderStageFlags(data.uniformsStages),
                .offset = 0,
                .size = static_cast<uint32_t>(data.uniformsSize),
            };
        }
        else
        {
            uniformBinding = {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = GetShaderStageFlags(data.uniformsStages),
            };
            uniformBufferSize = data.uniformsSize;
        }
    }

    const auto textureBindings = std::views::transform(std::views::iota(0u, data.textureCount), [](uint32 i) {
        return vk::DescriptorSetLayoutBinding{
            .binding = i + 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
        };
    });

    const vkex::DescriptorSetLayoutCreateInfo layoutInfo = {
        .bindings = vkex::Join(uniformBinding, textureBindings),
    };
    std::ranges::transform(layoutInfo.bindings, std::back_inserter(descriptorTypeCounts), [](const auto& binding) {
        return DescriptorTypeCount{binding.descriptorType, binding.descriptorCount};
    });
    setLayout = device.createDescriptorSetLayoutUnique(layoutInfo, s_allocator);
    uniformsStages = GetShaderStageFlags(data.uniformsStages);
}

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
