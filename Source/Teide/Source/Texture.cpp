
#include "Teide/Texture.h"

std::size_t Texture::CalculateMinSizeInBytes() const
{
	const auto pixelSize = GetFormatElementSize(format);

	std::size_t result = 0;
	vk::Extent2D mipExtent = size;

	for (auto i = 0u; i < mipLevelCount; i++)
	{
		result += mipExtent.width * mipExtent.height * pixelSize;
		mipExtent.width = std::max(1u, mipExtent.width / 2);
		mipExtent.height = std::max(1u, mipExtent.height / 2);
	}

	return result;
}

void Texture::GenerateMipmaps(TextureState& state, vk::CommandBuffer cmdBuffer)
{
	const auto makeBarrier = [&](vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
	                             vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel) {
		return vk::ImageMemoryBarrier{
		    .srcAccessMask = srcAccessMask,
		    .dstAccessMask = dstAccessMask,
		    .oldLayout = oldLayout,
		    .newLayout = newLayout,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image = image.get(),
		    .subresourceRange = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .baseMipLevel = mipLevel,
		        .levelCount = 1,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    }};
	};

	const auto origin = vk::Offset3D{0, 0, 0};
	auto prevMipSize = vk::Offset3D{
	    static_cast<int32_t>(size.width),
	    static_cast<int32_t>(size.height),
	    static_cast<int32_t>(1),
	};

	// Iterate all mip levels starting at 1
	for (uint32_t i = 1; i < mipLevelCount; i++)
	{
		const auto currMipSize = vk::Offset3D{
		    prevMipSize.x > 1 ? prevMipSize.x / 2 : 1,
		    prevMipSize.y > 1 ? prevMipSize.y / 2 : 1,
		    prevMipSize.z > 1 ? prevMipSize.z / 2 : 1,
		};

		// Transition previous mip level to be a transfer source
		const auto beforeBarrier = makeBarrier(
		    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eTransferDstOptimal,
		    vk::ImageLayout::eTransferSrcOptimal, i - 1);

		cmdBuffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, beforeBarrier);

		// Blit previous mip to current mip
		const auto blit = vk::ImageBlit{
		    .srcSubresource = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .mipLevel = i - 1,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    },
		    .srcOffsets = {{origin, prevMipSize}},
		    .dstSubresource = {
		        .aspectMask = vk::ImageAspectFlagBits::eColor,
		        .mipLevel = i,
		        .baseArrayLayer = 0,
		        .layerCount = 1,
		    },
		    .dstOffsets = {{origin, currMipSize}},
		};

		cmdBuffer.blitImage(
		    image.get(), vk::ImageLayout::eTransferSrcOptimal, image.get(), vk::ImageLayout::eTransferDstOptimal, blit,
		    vk::Filter::eLinear);

		// Transition previous mip level to be ready for reading in shader
		const auto afterBarrier = makeBarrier(
		    vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferSrcOptimal,
		    vk::ImageLayout::eShaderReadOnlyOptimal, i - 1);

		cmdBuffer.pipelineBarrier(
		    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, afterBarrier);

		prevMipSize = currMipSize;
	}

	// Transition final mip level to be ready for reading in shader
	const auto finalBarrier = makeBarrier(
	    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal,
	    vk::ImageLayout::eShaderReadOnlyOptimal, mipLevelCount - 1);

	cmdBuffer.pipelineBarrier(
	    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, finalBarrier);

	state.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void Texture::TransitionToShaderInput(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
	auto newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	if (state.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
	}
	DoTransition(state, cmdBuffer, newLayout, vk::PipelineStageFlagBits::eFragmentShader);
}

void Texture::TransitionToTransferSrc(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
	DoTransition(state, cmdBuffer, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer);
}

void Texture::DoTransition(
    TextureState& state, vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout,
    vk::PipelineStageFlags newPipelineStageFlags) const
{
	if (newLayout == state.layout && newPipelineStageFlags == state.lastPipelineStageUsage)
	{
		return;
	}

	TransitionImageLayout(
	    cmdBuffer, image.get(), format, mipLevelCount, state.layout, newLayout, state.lastPipelineStageUsage,
	    newPipelineStageFlags);

	state.layout = newLayout;
	state.lastPipelineStageUsage = newPipelineStageFlags;
}

void RenderableTexture::TransitionToRenderTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
	if (HasDepthOrStencilComponent(format))
	{
		TransitionToDepthStencilTarget(state, cmdBuffer);
	}
	else
	{
		TransitionToColorTarget(state, cmdBuffer);
	}
}

void RenderableTexture::TransitionToColorTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
	assert(!HasDepthOrStencilComponent(format));

	DoTransition(state, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

void RenderableTexture::TransitionToDepthStencilTarget(TextureState& state, vk::CommandBuffer cmdBuffer) const
{
	assert(HasDepthOrStencilComponent(format));

	DoTransition(
	    state, cmdBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal,
	    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);
}
