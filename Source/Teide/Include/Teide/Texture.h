
#pragma once

#include "Teide/BytesView.h"
#include "Teide/Internal/MemoryAllocator.h"
#include "Teide/Internal/Vulkan.h"

#include <taskflow/core/taskflow.hpp>

#include <vector>

struct TextureData
{
	vk::Extent2D size;
	vk::Format format;
	uint32_t mipLevelCount = 1;
	vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
	vk::SamplerCreateInfo samplerInfo;
	std::vector<std::byte> pixels;
};

struct TextureState
{
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::PipelineStageFlags lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe;
};

struct Texture
{
	vk::UniqueImage image;
	MemoryAllocation memory;
	vk::UniqueImageView imageView;
	vk::UniqueSampler sampler;
	vk::Extent2D size;
	vk::Format format;
	uint32_t mipLevelCount = 1;
	vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

	std::size_t CalculateMinSizeInBytes() const;

	void GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer);

	void TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer) const;
	void TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const;

protected:
	void DoTransition(
	    TextureState& state, vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout,
	    vk::PipelineStageFlags newPipelineStageFlags) const;
};

struct RenderableTexture : public Texture
{
	vk::UniqueRenderPass renderPass;
	vk::UniqueFramebuffer framebuffer;

	void TransitionToRenderTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
	void TransitionToColorTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
	void TransitionToDepthStencilTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const;
};
