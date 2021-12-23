
#include "Framework/Texture.h"

void Texture::DiscardContents()
{
	layout = vk::ImageLayout::eUndefined;
	lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe;
}

void Texture::GenerateMipmaps(vk::CommandBuffer cmdBuffer)
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

	layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void Texture::TransitionToShaderInput(vk::CommandBuffer cmdBuffer)
{
	auto newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
	}
	DoTransition(cmdBuffer, newLayout, vk::PipelineStageFlagBits::eFragmentShader);
}

void Texture::TransitionToColorTarget(vk::CommandBuffer cmdBuffer)
{
	assert(!HasDepthOrStencilComponent(format));

	DoTransition(cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput);
}

void Texture::TransitionToDepthStencilTarget(vk::CommandBuffer cmdBuffer)
{
	assert(HasDepthOrStencilComponent(format));

	DoTransition(
	    cmdBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal,
	    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);
}

void Texture::DoTransition(vk::CommandBuffer cmdBuffer, vk::ImageLayout newLayout, vk::PipelineStageFlags newPipelineStageFlags)
{
	if (newLayout == layout && newPipelineStageFlags == lastPipelineStageUsage)
	{
		return;
	}

	TransitionImageLayout(
	    cmdBuffer, image.get(), format, mipLevelCount, layout, newLayout, lastPipelineStageUsage, newPipelineStageFlags);

	layout = newLayout;
	lastPipelineStageUsage = newPipelineStageFlags;
}
