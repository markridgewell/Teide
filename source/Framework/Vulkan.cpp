
#include "Framework/Vulkan.h"

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
	}

	assert(false && "Unsupported image transition");
	return {};
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

vk::UniqueFence CreateFence(vk::Device device)
{
	return device.createFenceUnique(vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}, s_allocator);
}

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device)
{
	const auto createInfo = vk::CommandPoolCreateInfo{
	    .queueFamilyIndex = queueFamilyIndex,
	};

	return device.createCommandPoolUnique(createInfo, s_allocator);
}

CommandBuffer::CommandBuffer(vk::UniqueCommandBuffer commandBuffer) : m_cmdBuffer(std::move(commandBuffer))
{}

void CommandBuffer::TakeOwnership(vk::UniqueBuffer buffer)
{
	m_ownedBuffers.push_back(std::move(buffer));
}

void CommandBuffer::Reset()
{
	m_ownedBuffers.clear();
}

CommandBuffer::operator vk::CommandBuffer() const
{
	assert(m_cmdBuffer);
	return m_cmdBuffer.get();
}
OneShotCommandBuffer::OneShotCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue) :
    m_queue{queue}
{
	const auto allocInfo = vk::CommandBufferAllocateInfo{
	    .commandPool = commandPool,
	    .level = vk::CommandBufferLevel::ePrimary,
	    .commandBufferCount = 1,
	};
	auto cmdBuffers = device.allocateCommandBuffersUnique(allocInfo);
	m_cmdBuffer = std::move(cmdBuffers.front());

	m_cmdBuffer->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

OneShotCommandBuffer::~OneShotCommandBuffer()
{
	m_cmdBuffer->end();

	const auto submitInfo = vk::SubmitInfo{}.setCommandBuffers(m_cmdBuffer.get());
	m_queue.submit(submitInfo);
	m_queue.waitIdle();

	m_cmdBuffer.reset();
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
