
#include "Framework/Internal/Vulkan.h"

namespace
{
const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

struct TransitionAccessMasks
{
	vk::AccessFlags source;
	vk::AccessFlags destination;
};

vk::AccessFlags GetTransitionAccessMask(vk::ImageLayout layout)
{
	using enum vk::ImageLayout;
	using Access = vk::AccessFlagBits;

	switch (layout)
	{
		case eUndefined:
			return {};

		case eTransferDstOptimal:
			return Access::eTransferWrite;

		case eColorAttachmentOptimal:
			return Access::eColorAttachmentRead | Access::eColorAttachmentWrite;

		case eDepthStencilAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eDepthAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eStencilAttachmentOptimal:
			return Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite;

		case eShaderReadOnlyOptimal:
			return Access::eShaderRead;

		case eTransferSrcOptimal:
			return Access::eTransferRead;

		case eDepthStencilReadOnlyOptimal:
			return Access::eShaderRead;

		default:
			assert(false && "Unsupported image transition");
			return {};
	}
}

TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	return {GetTransitionAccessMask(oldLayout), GetTransitionAccessMask(newLayout)};
}

vk::ImageAspectFlags GetAspectMask(vk::Format format)
{
	if (HasDepthComponent(format))
	{
		if (HasStencilComponent(format))
		{
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		}
		else
		{
			return vk::ImageAspectFlagBits::eDepth;
		}
	}
	else if (HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eStencil;
	}
	else
	{
		return vk::ImageAspectFlagBits::eColor;
	}
}
} // namespace

bool HasDepthComponent(vk::Format format)
{
	return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool HasStencilComponent(vk::Format format)
{
	return format == vk::Format::eS8Uint || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool HasDepthOrStencilComponent(vk::Format format)
{
	return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat || format == vk::Format::eD16UnormS8Uint
	    || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eS8Uint;
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, vk::Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask)
{
	const auto accessMasks = GetTransitionAccessMasks(oldLayout, newLayout);

	const auto barrier = vk::ImageMemoryBarrier{
	    .srcAccessMask = accessMasks.source,
	    .dstAccessMask = accessMasks.destination,
	    .oldLayout = oldLayout,
	    .newLayout = newLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = image,
	    .subresourceRange = {
	        .aspectMask = GetAspectMask(format),
	        .baseMipLevel = 0,
	        .levelCount = mipLevelCount,
	        .baseArrayLayer = 0,
	        .layerCount = 1,
	    },
	};

	cmdBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}

vk::UniqueSemaphore CreateSemaphore(vk::Device device)
{
	return device.createSemaphoreUnique(vk::SemaphoreCreateInfo{}, s_allocator);
}

vk::UniqueFence CreateFence(vk::Device device, vk::FenceCreateFlags flags)
{
	return device.createFenceUnique(vk::FenceCreateInfo{.flags = flags}, s_allocator);
}

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device)
{
	const auto createInfo = vk::CommandPoolCreateInfo{
	    .queueFamilyIndex = queueFamilyIndex,
	};

	return device.createCommandPoolUnique(createInfo, s_allocator);
}

vk::ImageAspectFlags GetImageAspect(vk::Format format)
{
	if (HasDepthComponent(format) && HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}
	if (HasDepthComponent(format))
	{
		return vk::ImageAspectFlagBits::eDepth;
	}
	if (HasStencilComponent(format))
	{
		return vk::ImageAspectFlagBits::eStencil;
	}
	return vk::ImageAspectFlagBits::eColor;
}

void CopyBufferToImage(vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, vk::Format imageFormat, vk::Extent3D imageExtent)
{
	const auto copyRegion = vk::BufferImageCopy
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = GetImageAspect(imageFormat),
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0,0,0},
		.imageExtent = imageExtent,
	};
	cmdBuffer.copyBufferToImage(source, destination, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void CopyImageToBuffer(vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, vk::Format imageFormat, vk::Extent3D imageExtent)
{
	const auto copyRegion = vk::BufferImageCopy
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = GetImageAspect(imageFormat),
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0,0,0},
		.imageExtent = imageExtent,
	};
	cmdBuffer.copyImageToBuffer(source, vk::ImageLayout::eTransferSrcOptimal, destination, copyRegion);
}

std::uint32_t GetFormatElementSize(vk::Format format)
{
	switch (static_cast<VkFormat>(format))
	{
		default:
			assert("Unknown format!");
			return 0;

		/*
			8-bit
			Block size 1 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R4G4_UNORM_PACK8:
			return 1;
		case VK_FORMAT_R8_UNORM:
			return 1;
		case VK_FORMAT_R8_SNORM:
			return 1;
		case VK_FORMAT_R8_USCALED:
			return 1;
		case VK_FORMAT_R8_SSCALED:
			return 1;
		case VK_FORMAT_R8_UINT:
			return 1;
		case VK_FORMAT_R8_SINT:
			return 1;
		case VK_FORMAT_R8_SRGB:
			return 1;

		/*
			16-bit
			Block size 2 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			return 2;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
			return 2;
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			return 2;
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
			return 2;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return 2;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
			return 2;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return 2;
		case VK_FORMAT_R8G8_UNORM:
			return 2;
		case VK_FORMAT_R8G8_SNORM:
			return 2;
		case VK_FORMAT_R8G8_USCALED:
			return 2;
		case VK_FORMAT_R8G8_SSCALED:
			return 2;
		case VK_FORMAT_R8G8_UINT:
			return 2;
		case VK_FORMAT_R8G8_SINT:
			return 2;
		case VK_FORMAT_R8G8_SRGB:
			return 2;
		case VK_FORMAT_R16_UNORM:
			return 2;
		case VK_FORMAT_R16_SNORM:
			return 2;
		case VK_FORMAT_R16_USCALED:
			return 2;
		case VK_FORMAT_R16_SSCALED:
			return 2;
		case VK_FORMAT_R16_UINT:
			return 2;
		case VK_FORMAT_R16_SINT:
			return 2;
		case VK_FORMAT_R16_SFLOAT:
			return 2;
		case VK_FORMAT_R10X6_UNORM_PACK16:
			return 2;
		case VK_FORMAT_R12X4_UNORM_PACK16:
			return 2;

		/*
			24-bit
			Block size 3 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R8G8B8_UNORM:
			return 3;
		case VK_FORMAT_R8G8B8_SNORM:
			return 3;
		case VK_FORMAT_R8G8B8_USCALED:
			return 3;
		case VK_FORMAT_R8G8B8_SSCALED:
			return 3;
		case VK_FORMAT_R8G8B8_UINT:
			return 3;
		case VK_FORMAT_R8G8B8_SINT:
			return 3;
		case VK_FORMAT_R8G8B8_SRGB:
			return 3;
		case VK_FORMAT_B8G8R8_UNORM:
			return 3;
		case VK_FORMAT_B8G8R8_SNORM:
			return 3;
		case VK_FORMAT_B8G8R8_USCALED:
			return 3;
		case VK_FORMAT_B8G8R8_SSCALED:
			return 3;
		case VK_FORMAT_B8G8R8_UINT:
			return 3;
		case VK_FORMAT_B8G8R8_SINT:
			return 3;
		case VK_FORMAT_B8G8R8_SRGB:
			return 3;

		/*
			32-bit
			Block size 4 byte
			1x1x1 block extent
			1 texel/block
		*/
		case VK_FORMAT_R8G8B8A8_UNORM:
			return 4;
		case VK_FORMAT_R8G8B8A8_SNORM:
			return 4;
		case VK_FORMAT_R8G8B8A8_USCALED:
			return 4;
		case VK_FORMAT_R8G8B8A8_SSCALED:
			return 4;
		case VK_FORMAT_R8G8B8A8_UINT:
			return 4;
		case VK_FORMAT_R8G8B8A8_SINT:
			return 4;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 4;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return 4;
		case VK_FORMAT_B8G8R8A8_SNORM:
			return 4;
		case VK_FORMAT_B8G8R8A8_USCALED:
			return 4;
		case VK_FORMAT_B8G8R8A8_SSCALED:
			return 4;
		case VK_FORMAT_B8G8R8A8_UINT:
			return 4;
		case VK_FORMAT_B8G8R8A8_SINT:
			return 4;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return 4;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			return 4;
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			return 4;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			return 4;
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return 4;
		case VK_FORMAT_R16G16_UNORM:
			return 4;
		case VK_FORMAT_R16G16_SNORM:
			return 4;
		case VK_FORMAT_R16G16_USCALED:
			return 4;
		case VK_FORMAT_R16G16_SSCALED:
			return 4;
		case VK_FORMAT_R16G16_UINT:
			return 4;
		case VK_FORMAT_R16G16_SINT:
			return 4;
		case VK_FORMAT_R16G16_SFLOAT:
			return 4;
		case VK_FORMAT_R32_UINT:
			return 4;
		case VK_FORMAT_R32_SINT:
			return 4;
		case VK_FORMAT_R32_SFLOAT:
			return 4;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return 4;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return 4;
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
			return 4;
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
			return 4;
	}
}
