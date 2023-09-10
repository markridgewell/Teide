
#pragma once

#include "MemoryAllocator.h"
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

struct VulkanTextureData
{
    vk::UniqueImage image;
    MemoryAllocation memory;
    vma::UniqueAllocation allocation;
    vk::UniqueImageView imageView;
    vk::UniqueSampler sampler;
    Geo::Size2i size;
    Format format = Format::Unknown;
    uint32 mipLevelCount = 1;
    uint32 sampleCount = 1;
};

struct VulkanTexture : public Texture, VulkanTextureData
{
    explicit VulkanTexture(VulkanTextureData data) : VulkanTextureData{std::move(data)} {}

    Geo::Size2i GetSize() const override { return size; }
    Format GetFormat() const override { return format; }
    uint32 GetMipLevelCount() const override { return mipLevelCount; }
    uint32 GetSampleCount() const override { return sampleCount; }

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
