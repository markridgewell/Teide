
#pragma once

#include "Vulkan.h"

#include "GeoLib/Vector.h"
#include "Teide/Texture.h"

namespace Teide
{

struct TextureState
{
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::PipelineStageFlags lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe;
};

struct VulkanTexture
{
    vk::UniqueImage image;
    vma::UniqueAllocation allocation;
    vk::UniqueImageView imageView;
    vk::UniqueSampler sampler;
    TextureProperties properties;

    void GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer);

    void TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToTransferDst(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToRenderTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToColorTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToDepthStencilTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToPresentSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const;

protected:
    void DoTransition(
        TextureState& state, vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout,
        vk::PipelineStageFlags newPipelineStageFlags) const;
};

template <>
struct VulkanImpl<Texture>
{
    using type = VulkanTexture;
};

} // namespace Teide
