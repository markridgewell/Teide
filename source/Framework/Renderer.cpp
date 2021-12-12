
#pragma once

#include "Framework/Renderer.h"

#include "Framework/Vulkan.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

} // namespace

Renderer::Renderer(vk::Device device, uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex, uint32_t numThreads) :
    m_device{device},
    m_graphicsQueue{device.getQueue(graphicsFamilyIndex, 0)},
    m_presentQueue{device.getQueue(presentFamilyIndex, 0)},
    m_executor(numThreads)
{
	std::ranges::generate(
	    m_frameResources, [=] { return CreateThreadResources(m_device, graphicsFamilyIndex, numThreads); });
	std::ranges::generate(m_renderFinished, [=] { return CreateSemaphore(device); });
	std::ranges::generate(m_inFlightFences, [=] { return CreateFence(device); });
}

vk::Fence Renderer::BeginFrame(uint32_t frameNumber)
{
	constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

	assert(m_frameNumber == 0 || (m_frameNumber + 1) % MaxFramesInFlight == frameNumber);
	m_frameNumber = frameNumber;

	[[maybe_unused]] const auto waitResult = m_device.waitForFences(m_inFlightFences[frameNumber].get(), true, timeout);
	assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

	m_taskflow.clear();
	m_nextSequenceIndex = 0;

	auto& frameResources = m_frameResources[frameNumber];
	for (auto& threadResources : frameResources)
	{
		threadResources.Reset(m_device);
	}

	return m_inFlightFences[frameNumber].get();
}

void Renderer::EndFrame(std::span<const SurfaceImage> images)
{
	auto swapchains = std::vector<vk::SwapchainKHR>(images.size());
	auto imageIndices = std::vector<uint32_t>(images.size());
	auto waitSemaphores = std::vector<vk::Semaphore>(images.size());
	auto waitStages = std::vector<vk::PipelineStageFlags>(images.size());

	for (size_t i = 0; i < images.size(); i++)
	{
		swapchains[i] = images[i].swapchain;
		imageIndices[i] = images[i].imageIndex;
		waitSemaphores[i] = images[i].imageAvailable;
		waitStages[i] = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}

	auto signalSemaphores = std::span(&m_renderFinished[m_frameNumber].get(), 1);
	auto fenceToSignal = m_inFlightFences[m_frameNumber].get();

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

	m_device.resetFences(fenceToSignal);

	const auto submitInfo = vk::SubmitInfo{
	    .waitSemaphoreCount = size32(waitSemaphores),
	    .pWaitSemaphores = data(waitSemaphores),
	    .pWaitDstStageMask = data(waitStages),
	    .commandBufferCount = size32(commandBuffersToSubmit),
	    .pCommandBuffers = data(commandBuffersToSubmit),
	    .signalSemaphoreCount = size32(signalSemaphores),
	    .pSignalSemaphores = data(signalSemaphores),
	};
	m_graphicsQueue.submit(submitInfo, fenceToSignal);

	// Present
	const auto presentInfo = vk::PresentInfoKHR{
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores = &m_renderFinished[m_frameNumber].get(),
	    .swapchainCount = size32(swapchains),
	    .pSwapchains = data(swapchains),
	    .pImageIndices = data(imageIndices),
	};

	const auto presentResult = m_presentQueue.presentKHR(presentInfo);
	if (presentResult == vk::Result::eSuboptimalKHR)
	{
		spdlog::warn("Suboptimal swapchain image");
	}
}

void Renderer::EndFrame(const SurfaceImage& image)
{
	EndFrame(std::span(&image, 1));
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

void Renderer::RenderToTexture(
    vk::Image texture, vk::Format format, vk::Framebuffer framebuffer, vk::Extent2D framebufferSize, RenderList renderList)
{
	m_taskflow.emplace([=, this, renderList = std::move(renderList), sequenceIndex = m_nextSequenceIndex++] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		const vk::CommandBuffer commandBuffer = GetCommandBuffer(taskIndex, sequenceIndex);

		TransitionImageLayout(
		    commandBuffer, texture, format, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
		    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests);

		Render(renderList, framebuffer, framebufferSize, commandBuffer);

		TransitionImageLayout(
		    commandBuffer, texture, format, 1, vk::ImageLayout::eDepthStencilAttachmentOptimal,
		    vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::PipelineStageFlagBits::eLateFragmentTests,
		    vk::PipelineStageFlagBits::eFragmentShader);
	});
}

void Renderer::RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList)
{
	const auto framebuffer = surfaceImage.framebuffer;
	const auto extent = surfaceImage.extent;
	m_taskflow.emplace([=, this, renderList = std::move(renderList), sequenceIndex = m_nextSequenceIndex++] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		const vk::CommandBuffer commandBuffer = GetCommandBuffer(taskIndex, sequenceIndex);

		Render(renderList, framebuffer, extent, commandBuffer);
	});
}

void Renderer::Render(const RenderList& renderList, vk::Framebuffer framebuffer, vk::Extent2D framebufferSize, vk::CommandBuffer commandBuffer)
{
	using std::data;

	assert(renderList.objects.size() >= 1u);

	const auto renderPassBegin = vk::RenderPassBeginInfo{
	    .renderPass = renderList.renderPass,
	    .framebuffer = framebuffer,
	    .renderArea = {.offset = {0, 0}, .extent = framebufferSize},
	    .clearValueCount = size32(renderList.clearValues),
	    .pClearValues = data(renderList.clearValues),
	};

	commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

	const auto viewport = vk::Viewport{
	    .x = 0.0f,
	    .y = 0.0f,
	    .width = static_cast<float>(framebufferSize.width),
	    .height = static_cast<float>(framebufferSize.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f,
	};
	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, vk::Rect2D{.extent = framebufferSize});

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
