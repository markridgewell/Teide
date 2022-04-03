
#pragma once

#include <Framework/BytesView.h>
#include <Framework/MemoryAllocator.h>
#include <Framework/Vulkan.h>
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
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::PipelineStageFlags lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe;

	void GenerateMipmaps(vk::CommandBuffer cmdBuffer);

	void TransitionToShaderInput(vk::CommandBuffer cmdBuffer);

protected:
	void DoTransition(vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout, vk::PipelineStageFlags newPipelineStageFlags);
};

struct RenderableTexture : public Texture
{
	vk::UniqueRenderPass renderPass;
	vk::UniqueFramebuffer framebuffer;
	tf::Future<void> future;

	void DiscardContents();

	void TransitionToColorTarget(vk::CommandBuffer cmdBuffer);
	void TransitionToDepthStencilTarget(vk::CommandBuffer cmdBuffer);
	void TransitionToTransferSrc(vk::CommandBuffer cmdBuffer);
};
