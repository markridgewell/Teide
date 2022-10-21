
#pragma once

#include "GeoLib/Vector.h"
#include "MemoryAllocator.h"
#include "Teide/Texture.h"
#include "Vulkan.h"

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
	vk::UniqueImageView imageView;
	vk::UniqueSampler sampler;
	Geo::Size2i size;
	Format format;
	std::uint32_t mipLevelCount = 1;
	std::uint32_t sampleCount = 1;
};

struct VulkanTexture : public Texture, VulkanTextureData
{
	VulkanTexture(VulkanTextureData data) : VulkanTextureData{std::move(data)} {}

	Geo::Size2i GetSize() const override { return size; }
	Format GetFormat() const override { return format; }
	std::uint32_t GetMipLevelCount() const override { return mipLevelCount; }
	std::uint32_t GetSampleCount() const override { return sampleCount; }

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
