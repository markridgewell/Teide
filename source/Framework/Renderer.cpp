
#include "Framework/Renderer.h"

#include "Framework/GraphicsDevice.h"
#include "Framework/Texture.h"
#include "Framework/Vulkan.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

void SetCommandBufferDebugName(vk::UniqueCommandBuffer& commandBuffer, std::uint32_t threadIndex, std::uint32_t cbIndex)
{
	SetDebugName(commandBuffer, "RenderThread{}:CommandBuffer{}", threadIndex, cbIndex);
}

} // namespace

Renderer::Renderer(GraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex, uint32_t numThreads) :
    m_device{device}, m_graphicsQueue{device.GetVulkanDevice().getQueue(graphicsFamilyIndex, 0)}, m_executor(numThreads)
{
	const auto vkdevice = device.GetVulkanDevice();

	if (presentFamilyIndex)
	{
		m_presentQueue = vkdevice.getQueue(*presentFamilyIndex, 0);
	}

	std::ranges::generate(
	    m_frameResources, [=] { return CreateThreadResources(vkdevice, graphicsFamilyIndex, numThreads); });
	std::ranges::generate(m_renderFinished, [=] { return CreateSemaphore(vkdevice); });
	std::ranges::generate(m_inFlightFences, [=] { return CreateFence(vkdevice, vk::FenceCreateFlagBits::eSignaled); });

	m_schedulerThread = std::jthread([this, stop_token = m_schedulerStopSource.get_token()] {
		constexpr auto timeout = std::chrono::milliseconds{2};

		std::vector<vk::Fence> fences;
		const auto device = m_device.GetVulkanDevice();

		while (!stop_token.stop_requested())
		{
			fences.clear();
			{
				auto lock = std::scoped_lock(m_schedulerMutex);

				// Make a list of fence handles of all scheduled tasks
				std::ranges::transform(m_scheduledTasks, std::back_inserter(fences), &ScheduledTask::fence);
			}

			// If there are no fences to wait on, sleep for a bit and try again
			if (fences.empty())
			{
				std::this_thread::sleep_for(timeout);
				continue;
			}

			// If there are fences, wait on them and see if any are signalled
			if (device.waitForFences(fences, false, Timeout(timeout)) == vk::Result::eSuccess)
			{
				auto lock = std::scoped_lock(m_schedulerMutex);

				// A fence was signalled, but we don't know which one yet, iterate them to find out
				for (auto& [fence, callback] : m_scheduledTasks)
				{
					if (device.waitForFences(fence, false, 0) == vk::Result::eSuccess)
					{
						// Found one, execute the attached callback
						LaunchTask(callback);

						// Mark it as done
						callback = nullptr;
					}
				}

				// Remove done tasks
				std::erase_if(m_scheduledTasks, [](const auto& entry) { return entry.callback == nullptr; });
			}
		}
	});
}

Renderer::~Renderer()
{
	m_schedulerStopSource.request_stop();

	WaitForTasks();

	auto fences = std::vector<vk::Fence>();
	std::ranges::transform(m_inFlightFences, std::back_inserter(fences), [](const auto& f) { return f.get(); });
	std::ranges::transform(m_submitFences, std::back_inserter(fences), [](const auto& f) { return f.get(); });

	constexpr auto timeout = Timeout(std::chrono::seconds{1});
	if (m_device.GetVulkanDevice().waitForFences(fences, true, timeout) == vk::Result::eTimeout)
	{
		spdlog::error("Timeout while waiting for command buffer execution to complete!");
	}
}

std::uint32_t Renderer::GetFrameNumber() const
{
	return m_frameNumber;
}

vk::Fence Renderer::BeginFrame()
{
	constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

	m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

	[[maybe_unused]] const auto waitResult
	    = m_device.GetVulkanDevice().waitForFences(m_inFlightFences[m_frameNumber].get(), true, timeout);
	assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

	auto& frameResources = m_frameResources[m_frameNumber];
	for (auto& threadResources : frameResources)
	{
		threadResources.Reset(m_device.GetVulkanDevice());
	}

	return m_inFlightFences[m_frameNumber].get();
}

void Renderer::EndFrame(std::span<const SurfaceImage> images)
{
	assert(m_presentQueue && "Can't present without a present queue");

	const auto device = m_device.GetVulkanDevice();

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

	WaitForTasks();

	device.resetFences(fenceToSignal);

	// Submit the surface command buffer(s)
	{
		const auto lock = std::scoped_lock(m_surfaceCommandBuffersMutex);
		const auto submitInfo = vk::SubmitInfo{
		    .waitSemaphoreCount = size32(waitSemaphores),
		    .pWaitSemaphores = data(waitSemaphores),
		    .pWaitDstStageMask = data(waitStages),
		    .commandBufferCount = size32(m_surfaceCommandBuffers),
		    .pCommandBuffers = data(m_surfaceCommandBuffers),
		    .signalSemaphoreCount = size32(signalSemaphores),
		    .pSignalSemaphores = data(signalSemaphores),
		};
		m_graphicsQueue.submit(submitInfo, fenceToSignal);

		m_surfaceCommandBuffers.clear();
	}

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

	// Reuse fences for finished submits
	for (auto& fence : m_submitFences)
	{
		if (device.waitForFences(fence.get(), false, 0) == vk::Result::eSuccess)
		{
			device.resetFences(fence.get());
			m_unusedSubmitFences.push(std::exchange(fence, {}));
		}
	}
	std::erase_if(m_submitFences, [](const vk::UniqueFence& fence) { return !fence; });
}

void Renderer::EndFrame(const SurfaceImage& image)
{
	EndFrame(std::span(&image, 1));
}

void Renderer::ThreadResources::Reset(vk::Device device)
{
	device.resetCommandPool(commandPool.get());
	numUsedCommandBuffers = 0;

	// Resetting also resets the command buffers' debug names
	for (std::uint32_t i = 0; i < commandBuffers.size(); i++)
	{
		commandBuffers[i].Reset();
		SetCommandBufferDebugName(commandBuffers[i].Get(), threadIndex, i);
	}
}

CommandBuffer& Renderer::GetCommandBuffer(uint32_t threadIndex)
{
	auto& threadResources = m_frameResources[m_frameNumber][threadIndex];
	auto device = m_device.GetVulkanDevice();

	if (threadResources.numUsedCommandBuffers == threadResources.commandBuffers.size())
	{
		const auto allocateInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = threadResources.commandPool.get(),
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = std::max(1u, static_cast<std::uint32_t>(threadResources.commandBuffers.size())),
		};

		auto newCommandBuffers = device.allocateCommandBuffersUnique(allocateInfo);
		const auto numCBs = static_cast<std::uint32_t>(threadResources.commandBuffers.size());
		for (std::uint32_t i = 0; i < newCommandBuffers.size(); i++)
		{
			SetCommandBufferDebugName(newCommandBuffers[i], threadIndex, i + numCBs);
			threadResources.commandBuffers.push_back(CommandBuffer(std::move(newCommandBuffers[i])));
		}
	}

	const auto commandBufferIndex = threadResources.numUsedCommandBuffers++;
	CommandBuffer& commandBuffer = threadResources.commandBuffers.at(commandBufferIndex);
	commandBuffer.Get()->begin(vk::CommandBufferBeginInfo{});

	return commandBuffer;
}

void Renderer::RenderToTexture(RenderableTexturePtr texture, RenderList renderList)
{
	const std::uint32_t sequenceIndex = AddCommandBufferSlot();

	LaunchTask([=, renderList = std::move(renderList), texture = std::move(texture)] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		CommandBuffer& commandBuffer = GetCommandBuffer(taskIndex);
		commandBuffer.AddTexture(texture);

		TextureState textureState;
		texture->TransitionToRenderTarget(textureState, commandBuffer);

		BuildCommandBuffer(commandBuffer, renderList, texture->renderPass.get(), texture->framebuffer.get(), texture->size);

		texture->TransitionToShaderInput(textureState, commandBuffer);

		SubmitCommandBuffer(sequenceIndex, commandBuffer);
	});
}

void Renderer::RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList)
{
	const auto framebuffer = surfaceImage.framebuffer;
	const auto extent = surfaceImage.extent;

	LaunchTask([=, renderList = std::move(renderList)] {
		const uint32_t taskIndex = m_executor.this_worker_id();

		CommandBuffer& commandBuffer = GetCommandBuffer(taskIndex);

		BuildCommandBuffer(commandBuffer, renderList, surfaceImage.renderPass, framebuffer, extent);

		commandBuffer.Get()->end();
		m_surfaceCommandBuffers.push_back(commandBuffer);
	});
}

std::future<TextureData> Renderer::CopyTextureData(RenderableTexturePtr texture)
{
	const std::uint32_t sequenceIndex = AddCommandBufferSlot();

	auto promise = std::make_shared<std::promise<TextureData>>();
	auto future = promise->get_future();

	LaunchTask([=, &texture, promise = std::move(promise)]() {
		const uint32_t taskIndex = m_executor.this_worker_id();

		auto buffer = CreateBufferUninitialized(
		    texture->memory.size, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible,
		    m_device.GetVulkanDevice(), m_device.GetMemoryAllocator());

		const auto& bufferData = buffer.allocation;

		CommandBuffer& commandBuffer = GetCommandBuffer(taskIndex);
		commandBuffer.AddTexture(texture);

		TextureState textureState = {
		    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
		};
		texture->TransitionToTransferSrc(textureState, commandBuffer);
		const auto extent = vk::Extent3D{texture->size.width, texture->size.height, 1};
		CopyImageToBuffer(commandBuffer, texture->image.get(), buffer.buffer.get(), texture->format, extent);
		commandBuffer.TakeOwnership(std::move(buffer.buffer));
		texture->TransitionToShaderInput(textureState, commandBuffer);

		const TextureData textureData = {
		    .size = texture->size,
		    .format = texture->format,
		    .mipLevelCount = texture->mipLevelCount,
		    .samples = texture->samples,
		};

		SubmitCommandBuffer(sequenceIndex, commandBuffer, [bufferData, textureData, promise]() {
			const auto data = static_cast<const std::byte*>(bufferData.mappedData);

			const auto size = textureData.size.width * textureData.size.height * GetFormatElementSize(textureData.format);

			TextureData ret = textureData;
			ret.pixels.resize(size);
			std::copy(data, data + size, ret.pixels.data());

			promise->set_value(ret);
		});
	});

	return future;
}

void Renderer::BuildCommandBuffer(
    CommandBuffer& commandBufferWrapper, const RenderList& renderList, vk::RenderPass renderPass,
    vk::Framebuffer framebuffer, vk::Extent2D framebufferSize)
{
	using std::data;

	vk::CommandBuffer commandBuffer = commandBufferWrapper;

	const auto renderPassBegin = vk::RenderPassBeginInfo{
	    .renderPass = renderPass,
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

	if (!renderList.objects.empty())
	{
		const auto descriptorSets = std::array{
		    GetDescriptorSet(renderList.sceneParameters.get()),
		    GetDescriptorSet(renderList.viewParameters.get()),
		};
		commandBuffer.bindDescriptorSets(
		    vk::PipelineBindPoint::eGraphics, renderList.objects.front().pipeline->layout, 0, descriptorSets, {});

		for (const RenderObject& obj : renderList.objects)
		{
			commandBufferWrapper.AddParameterBlock(obj.materialParameters);

			commandBuffer.bindDescriptorSets(
			    vk::PipelineBindPoint::eGraphics, obj.pipeline->layout, 2,
			    GetDescriptorSet(obj.materialParameters.get()), {});
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, obj.pipeline->pipeline.get());
			commandBuffer.bindVertexBuffers(0, obj.vertexBuffer->buffer.get(), vk::DeviceSize{0});
			commandBuffer.bindIndexBuffer(obj.indexBuffer->buffer.get(), vk::DeviceSize{0}, vk::IndexType::eUint16);
			commandBuffer.pushConstants(
			    obj.pipeline->layout, vk::ShaderStageFlagBits::eVertex, 0, size32(obj.pushConstants),
			    data(obj.pushConstants));
			commandBuffer.drawIndexed(obj.indexCount, 1, 0, 0, 0);
		}
	}

	commandBuffer.endRenderPass();
}

std::vector<Renderer::ThreadResources>
Renderer::CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads)
{
	std::vector<ThreadResources> ret;
	ret.resize(numThreads);
	int i = 0;
	std::ranges::generate(ret, [&] {
		ThreadResources res;
		res.commandPool = CreateCommandPool(queueFamilyIndex, device);
		SetDebugName(res.commandPool, "RenderThread{}:CommandPool", i);
		res.threadIndex = i;
		i++;
		return res;
	});

	return ret;
}

vk::DescriptorSet Renderer::GetDescriptorSet(const ParameterBlock* parameterBlock) const
{
	if (parameterBlock == nullptr)
	{
		return {};
	}
	return parameterBlock->descriptorSet[m_frameNumber % parameterBlock->descriptorSet.size()].get();
}

std::uint32_t Renderer::AddCommandBufferSlot()
{
	const auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

	m_readyCommandBuffers.emplace_back();
	return static_cast<std::uint32_t>(m_readyCommandBuffers.size() - 1);
}

void Renderer::SubmitCommandBuffer(std::uint32_t index, vk::CommandBuffer commandBuffer, CompletionHandler function)
{
	const auto getFence = [this] {
		if (m_unusedSubmitFences.empty())
		{
			return CreateFence(m_device.GetVulkanDevice());
		}

		auto nextFence = std::move(m_unusedSubmitFences.front());
		m_unusedSubmitFences.pop();
		return nextFence;
	};

	const auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

	if (function)
	{
		m_completionHandlers.emplace(commandBuffer, std::move(function));
	}

	commandBuffer.end();
	m_readyCommandBuffers.at(index) = commandBuffer;

	const auto unsubmittedCommandBuffers = std::span(m_readyCommandBuffers).subspan(m_numSubmittedCommandBuffers);

	const auto end = std::ranges::find(unsubmittedCommandBuffers, vk::CommandBuffer{});
	const auto commandBuffersToSubmit = std::span(unsubmittedCommandBuffers.begin(), end);

	if (!commandBuffersToSubmit.empty())
	{
		const auto fence = m_submitFences.emplace_back(getFence()).get();
		m_numSubmittedCommandBuffers += commandBuffersToSubmit.size();

		const auto submitInfo = vk::SubmitInfo{
		    .commandBufferCount = size32(commandBuffersToSubmit),
		    .pCommandBuffers = data(commandBuffersToSubmit),
		};
		m_graphicsQueue.submit(submitInfo, fence);

		// Queue up completion handlers
		for (vk::CommandBuffer cb : commandBuffersToSubmit)
		{
			const auto lock2 = std::scoped_lock(m_schedulerMutex);

			const auto completionHandler = m_completionHandlers.extract(cb);
			if (completionHandler)
			{
				m_scheduledTasks.emplace_back(fence, std::move(completionHandler.mapped()));
			}
		}
	}
}
