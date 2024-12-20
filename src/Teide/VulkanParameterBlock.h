
#pragma once

#include "Vulkan.h"
#include "VulkanBuffer.h"

#include "Teide/ParameterBlock.h"
#include "Teide/ShaderData.h"
#include "Teide/Texture.h"

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
    uint32 uniformBufferSize = 0;
    std::optional<vk::PushConstantRange> pushConstantRange;
    vk::ShaderStageFlags uniformsStages;

    VulkanParameterBlockLayout() = default;
    explicit VulkanParameterBlockLayout(const ParameterBlockLayoutData& data, vk::Device device);

    bool IsEmpty() const override;
    bool HasDescriptors() const;
    bool HasPushConstants() const;
};

using VulkanParameterBlockLayoutPtr = std::shared_ptr<const VulkanParameterBlockLayout>;

template <>
struct VulkanImpl<ParameterBlockLayout>
{
    using type = VulkanParameterBlockLayout;
};

struct VulkanParameterBlock
{
    std::shared_ptr<VulkanBuffer> uniformBuffer;
    std::vector<Texture> textures;
    vk::UniqueDescriptorSet descriptorSet;
    std::vector<byte> pushConstantData;

    VulkanParameterBlock() = default;
    explicit VulkanParameterBlock(const VulkanParameterBlockLayout& layout);

    usize GetUniformBufferSize() const;
    usize GetPushConstantSize() const;
};

struct TransientParameterBlock
{
    std::shared_ptr<VulkanBuffer> uniformBuffer;
    std::vector<Texture> textures;
    vk::DescriptorSet descriptorSet;
};

template <>
struct VulkanImpl<ParameterBlock>
{
    using type = VulkanParameterBlock;
};

} // namespace Teide
