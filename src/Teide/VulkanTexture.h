
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
    uint32 refCount = 1;

    vk::UniqueImage image;
    vma::UniqueAllocation allocation;
    vk::UniqueImageView imageView;
    vk::UniqueSampler sampler;
    Geo::Size2i size;
    Format format = Format::Unknown;
    uint32 mipLevelCount = 1;
    uint32 sampleCount = 1;

    void GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer);

    void TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer) const;
    void TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const;
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
