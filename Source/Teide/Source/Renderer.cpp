
#include "Framework/Renderer.h"

#include "Framework/GraphicsDevice.h"
#include "Framework/Internal/Vulkan.h"
#include "Framework/Texture.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

} // namespace

Renderer::Renderer(GraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex, uint32_t numThreads) :
    m_device{device},
    m_graphicsQueue{device.GetVulkanDevice().getQueue(graphicsFamilyIndex, 0)},
    m_scheduler(numThreads, m_device.GetVulkanDevice(), m_graphicsQueue, graphicsFamilyIndex)
{
	const auto vkdevice = device.GetVulkanDevice();

	if (presentFamilyIndex)
	{
		m_presentQueue = vkdevice.getQueue(*presentFamilyIndex, 0);
	}

	std::ranges::generate(m_renderFinished, [=] { return CreateSemaphore(vkdevice); });
	std::ranges::generate(m_inFlightFences, [=] { return CreateFence(vkdevice, vk::FenceCreateFlagBits::eSignaled); });
}

Renderer::~Renderer()
{
	auto fences = std::vector<vk::Fence>();
	std::ranges::transform(m_inFlightFences, std::back_inserter(fences), [](const auto& f) { return f.get(); });
	if (!fences.empty())
	{
		constexpr auto timeout = Timeout(std::chrono::seconds{1});
		if (m_device.GetVulkanDevice().waitForFences(fences, true, timeout) == vk::Result::eTimeout)
		{
			spdlog::error("Timeout while waiting for command buffer execution to complete!");
		}
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

	m_scheduler.NextFrame();

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

	m_scheduler.WaitForTasks();

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
}

void Renderer::EndFrame(const SurfaceImage& image)
{
	EndFrame(std::span(&image, 1));
}

void Renderer::RenderToTexture(RenderableTexturePtr texture, RenderList renderList)
{
	m_scheduler.ScheduleGpu([this, renderList = std::move(renderList),
	                         texture = std::move(texture)](CommandBuffer& commandBuffer) {
		commandBuffer.AddTexture(texture);

		TextureState textureState;
		texture->TransitionToRenderTarget(textureState, commandBuffer);

		BuildCommandBuffer(commandBuffer, renderList, texture->renderPass.get(), texture->framebuffer.get(), texture->size);

		texture->TransitionToShaderInput(textureState, commandBuffer);
	});
}

void Renderer::RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList)
{
	const auto framebuffer = surfaceImage.framebuffer;
	const auto extent = surfaceImage.extent;

	m_scheduler.Schedule([=, this, renderList = std::move(renderList)](uint32_t taskIndex) {
		CommandBuffer& commandBuffer = m_scheduler.GetCommandBuffer(taskIndex);

		BuildCommandBuffer(commandBuffer, renderList, surfaceImage.renderPass, framebuffer, extent);

		commandBuffer.Get()->end();

		const auto lock = std::scoped_lock(m_surfaceCommandBuffersMutex);
		m_surfaceCommandBuffers.push_back(commandBuffer);
	});
}

Task<TextureData> Renderer::CopyTextureData(RenderableTexturePtr texture)
{
	const TextureData textureData = {
	    .size = texture->size,
	    .format = texture->format,
	    .mipLevelCount = texture->mipLevelCount,
	    .samples = texture->samples,
	};

	auto task = m_scheduler.ScheduleGpu([this, texture = std::move(texture)](CommandBuffer& commandBuffer) {
		auto buffer = std::make_shared<Buffer>(CreateBufferUninitialized(
		    texture->memory.size, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible,
		    m_device.GetVulkanDevice(), m_device.GetMemoryAllocator()));

		commandBuffer.AddTexture(texture);

		TextureState textureState = {
		    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
		};
		texture->TransitionToTransferSrc(textureState, commandBuffer);
		const auto extent = vk::Extent3D{texture->size.width, texture->size.height, 1};
		CopyImageToBuffer(commandBuffer, texture->image.get(), buffer->buffer.get(), texture->format, extent);
		texture->TransitionToShaderInput(textureState, commandBuffer);

		return buffer;
	});

	return m_scheduler.ScheduleAfter(task, [textureData](const BufferPtr& buffer) {
		const auto data = static_cast<const std::byte*>(buffer->allocation.mappedData);

		const auto size = textureData.size.width * textureData.size.height * GetFormatElementSize(textureData.format);

		TextureData ret = textureData;
		ret.pixels.resize(size);
		std::copy(data, data + size, ret.pixels.data());
		return ret;
	});
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

vk::DescriptorSet Renderer::GetDescriptorSet(const ParameterBlock* parameterBlock) const
{
	if (parameterBlock == nullptr)
	{
		return {};
	}
	return parameterBlock->descriptorSet[m_frameNumber % parameterBlock->descriptorSet.size()].get();
}
