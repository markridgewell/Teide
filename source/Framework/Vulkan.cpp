
#include "Framework/Vulkan.h"

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

struct TransitionAccessMasks
{
	vk::AccessFlags source;
	vk::AccessFlags destination;
};

TransitionAccessMasks GetTransitionAccessMasks(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	using enum vk::ImageLayout;
	using Access = vk::AccessFlagBits;

	if (oldLayout == eUndefined && newLayout == eTransferDstOptimal)
	{
		return {{}, Access::eTransferWrite};
	}
	else if (oldLayout == eTransferDstOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eTransferWrite, Access::eShaderRead};
	}
	else if (oldLayout == eUndefined && newLayout == eColorAttachmentOptimal)
	{
		return {{}, Access::eColorAttachmentRead | Access::eColorAttachmentWrite};
	}
	else if (oldLayout == eUndefined && newLayout == eDepthAttachmentOptimal)
	{
		return {{}, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
	}
	else if (oldLayout == eUndefined && newLayout == eStencilAttachmentOptimal)
	{
		return {{}, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
	}
	else if (oldLayout == eUndefined && newLayout == eDepthStencilAttachmentOptimal)
	{
		return {{}, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
	}
	else if (oldLayout == eColorAttachmentOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eColorAttachmentWrite, Access::eShaderRead};
	}
	else if (oldLayout == eDepthStencilAttachmentOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite, Access::eShaderRead};
	}
	else if (oldLayout == eDepthStencilAttachmentOptimal && newLayout == eDepthStencilReadOnlyOptimal)
	{
		return {Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite, Access::eShaderRead};
	}
	else if (oldLayout == eDepthAttachmentOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite, Access::eShaderRead};
	}
	else if (oldLayout == eStencilAttachmentOptimal && newLayout == eShaderReadOnlyOptimal)
	{
		return {Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite, Access::eShaderRead};
	}
	else if (oldLayout == eShaderReadOnlyOptimal && newLayout == eDepthStencilAttachmentOptimal)
	{
		return {Access::eShaderRead, Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite};
	}

	assert(false && "Unsupported image transition");
	return {};
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
}
