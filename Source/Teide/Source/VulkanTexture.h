
#pragma once

#include "MemoryAllocator.h"
#include "Teide/Texture.h"
#include "Vulkan.h"

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
	vk::Extent2D size;
	vk::Format format;
	uint32_t mipLevelCount = 1;
	vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
};

struct VulkanTexture : public Texture, VulkanTextureData
{
	VulkanTexture(VulkanTextureData data) : VulkanTextureData{std::move(data)} {}

	vk::Extent2D GetSize() const override { return size; }
	vk::Format GetFormat() const override { return format; }
	std::uint32_t GetMipLevelCount() const override { return mipLevelCount; }
	vk::SampleCountFlagBits GetSampleCount() const override { return sampleCount; }

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
