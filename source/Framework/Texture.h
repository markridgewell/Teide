
#pragma once

#include "Framework/BytesView.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Vulkan.h"

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

std::vector<std::byte> CopyBytes(BytesView src);

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

	void GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer);

	void TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer);

protected:
	void DoTransition(
	    TextureState& state, vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout,
	    vk::PipelineStageFlags newPipelineStageFlags);
};

struct RenderableTexture : public Texture
{
	vk::UniqueRenderPass renderPass;
	vk::UniqueFramebuffer framebuffer;

	void TransitionToRenderTarget(TextureState& state, vk::CommandBuffer cmdBuffer);
	void TransitionToColorTarget(TextureState& state, vk::CommandBuffer cmdBuffer);
	void TransitionToDepthStencilTarget(TextureState& state, vk::CommandBuffer cmdBuffer);
	void TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer);
};
