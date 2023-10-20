
#pragma once

#include "Vulkan.h"

#include "Teide/ParameterBlock.h"

#include <memory>
#include <optional>
#include <vector>

namespace Teide
{

struct DescriptorTypeCount
{
    vk::DescriptorType type;
    uint32 count;
};

struct VulkanParameterBlockLayout : public ParameterBlockLayout
{
    std::vector<DescriptorTypeCount> descriptorTypeCounts;
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
    std::vector<TexturePtr> textures;
    vk::UniqueDescriptorSet descriptorSet;
    std::vector<byte> pushConstantData;

    explicit VulkanParameterBlock(const VulkanParameterBlockLayout& layout);

    usize GetUniformBufferSize() const override;
    usize GetPushConstantSize() const override;
};

template <>
struct VulkanImpl<ParameterBlock>
{
    using type = VulkanParameterBlock;
};

} // namespace Teide
