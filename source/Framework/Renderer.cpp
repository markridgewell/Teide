
#pragma once

#include "Framework/Renderer.h"

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device)
{
	const auto createInfo = vk::CommandPoolCreateInfo{
	    .queueFamilyIndex = queueFamilyIndex,
	};

	return device.createCommandPoolUnique(createInfo, s_allocator);
}
} // namespace

Renderer::Renderer(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads) :
    m_device{device}, m_queue{device.getQueue(queueFamilyIndex, 0)}, m_executor(numThreads)
{
	std::ranges::generate(m_frameResources, [=] { return CreateThreadResources(m_device, queueFamilyIndex, numThreads); });
}

void Renderer::BeginFrame(uint32_t frameNumber)
{
	assert(m_frameNumber == 0 || (m_frameNumber + 1) % MaxFramesInFlight == frameNumber);
	m_frameNumber = frameNumber;

	m_taskflow.clear();
	m_nextSequenceIndex = 0;

	auto& frameResources = m_frameResources[frameNumber];
	for (auto& threadResources : frameResources)
	{
		threadResources.Reset(m_device);
	}
}

void Renderer::EndFrame(vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, vk::Fence fence)
{
	m_executor.run(m_taskflow).wait();

	// Submit the command buffer(s)
	std::vector<vk::CommandBuffer> commandBuffersToSubmit;
	std::vector<uint32_t> sequenceIndices;
	for (const auto& threadResources : m_frameResources[m_frameNumber])
	{
		if (threadResources.usedThisFrame)
		{
			threadResources.commandBuffer->end();

			const auto posToInsert = std::ranges::upper_bound(sequenceIndices, threadResources.sequenceIndex);
			const auto index = std::distance(sequenceIndices.begin(), posToInsert);

			sequenceIndices.insert(posToInsert, threadResources.sequenceIndex);
			commandBuffersToSubmit.insert(commandBuffersToSubmit.begin() + index, threadResources.commandBuffer.get());
		}
	}

	m_device.resetFences(fence);

	const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	const auto submitInfo = vk::SubmitInfo{
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores = &waitSemaphore,
	    .pWaitDstStageMask = &waitStage,
	    .commandBufferCount = size32(commandBuffersToSubmit),
	    .pCommandBuffers = data(commandBuffersToSubmit),
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores = &signalSemaphore,
	};
	m_queue.submit(submitInfo, fence);
}

vk::CommandBuffer Renderer::GetCommandBuffer(uint32_t threadIndex, uint32_t sequenceIndex)
{
	auto& threadResources = m_frameResources[m_frameNumber][threadIndex];

	const vk::CommandBuffer commandBuffer = threadResources.commandBuffer.get();
	if (!threadResources.usedThisFrame)
	{
		commandBuffer.begin(vk::CommandBufferBeginInfo{});
		threadResources.usedThisFrame = true;
		threadResources.sequenceIndex = sequenceIndex;
	}

	return commandBuffer;
}

void Renderer::RenderToTexture(vk::Image texture, vk::Format format, RenderList renderList)
{
	m_taskflow.emplace([=, this, renderList = std::move(renderList), sequenceIndex = m_nextSequenceIndex++] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		const vk::CommandBuffer commandBuffer = GetCommandBuffer(taskIndex, sequenceIndex);

		TransitionImageLayout(
		    commandBuffer, texture, format, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
		    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests);

		Render(renderList, commandBuffer);

		TransitionImageLayout(
		    commandBuffer, texture, format, 1, vk::ImageLayout::eDepthStencilAttachmentOptimal,
		    vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::PipelineStageFlagBits::eLateFragmentTests,
		    vk::PipelineStageFlagBits::eFragmentShader);
	});
}

void Renderer::RenderToSurface(RenderList renderList)
{
	m_taskflow.emplace([=, this, renderList = std::move(renderList), sequenceIndex = m_nextSequenceIndex++] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		const vk::CommandBuffer commandBuffer = GetCommandBuffer(taskIndex, sequenceIndex);

		Render(renderList, commandBuffer);
	});
}

void Renderer::Render(const RenderList& renderList, vk::CommandBuffer commandBuffer)
{
	using std::data;

	assert(renderList.objects.size() >= 1u);

	const auto renderPassBegin = vk::RenderPassBeginInfo{
	    .renderPass = renderList.renderPass,
	    .framebuffer = renderList.framebuffer,
	    .renderArea = {.offset = {0, 0}, .extent = renderList.framebufferSize},
	    .clearValueCount = size32(renderList.clearValues),
	    .pClearValues = data(renderList.clearValues),
	};

	commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

	const auto sceneSet = GetDescriptorSet(renderList.sceneDescriptorSet);
	const auto viewSet = GetDescriptorSet(renderList.viewDescriptorSet);

	for (const RenderObject& obj : renderList.objects)
	{
		const auto descriptorSets = std::array{
		    sceneSet,
		    viewSet,
		    GetDescriptorSet(obj.materialDescriptorSet),
		};

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, obj.pipelineLayout, 0, descriptorSets, {});
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, obj.pipeline);
		commandBuffer.bindVertexBuffers(0, obj.vertexBuffer, vk::DeviceSize{0});
		commandBuffer.bindIndexBuffer(obj.indexBuffer, vk::DeviceSize{0}, vk::IndexType::eUint16);
		commandBuffer.pushConstants(
		    obj.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, size32(obj.pushConstants), data(obj.pushConstants));
		commandBuffer.drawIndexed(obj.indexCount, 1, 0, 0, 0);
	}

	commandBuffer.endRenderPass();
}

std::vector<Renderer::ThreadResources>
Renderer::CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads)
{
	std::vector<ThreadResources> ret;
	ret.resize(numThreads);
	std::ranges::generate(ret, [&] {
		ThreadResources res;
		res.commandPool = CreateCommandPool(queueFamilyIndex, device);
		const auto allocateInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = res.commandPool.get(),
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = 1,
		};
		res.commandBuffer = std::move(device.allocateCommandBuffersUnique(allocateInfo).front());
		return res;
	});

	return ret;
}

vk::DescriptorSet Renderer::GetDescriptorSet(const DescriptorSet* descriptorSet) const
{
	if (descriptorSet == nullptr)
		return nullptr;
	return descriptorSet->sets[m_frameNumber];
}
