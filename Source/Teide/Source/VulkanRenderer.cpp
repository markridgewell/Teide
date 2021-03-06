
#include "VulkanRenderer.h"

#include "Teide/Renderer.h"
#include "Types/TextureData.h"
#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanTexture.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <array>

namespace
{
static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

} // namespace

VulkanRenderer::VulkanRenderer(
    VulkanGraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex,
    uint32_t numThreads) :
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

VulkanRenderer::~VulkanRenderer()
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

std::uint32_t VulkanRenderer::GetFrameNumber() const
{
	return m_frameNumber;
}

void VulkanRenderer::BeginFrame()
{
	constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

	m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

	[[maybe_unused]] const auto waitResult
	    = m_device.GetVulkanDevice().waitForFences(m_inFlightFences[m_frameNumber].get(), true, timeout);
	assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

	m_scheduler.NextFrame();
}

void VulkanRenderer::EndFrame()
{
	std::vector<SurfaceImage> images;
	{
		const auto lock = std::scoped_lock(m_surfaceCommandBuffersMutex);
		std::swap(images, m_surfacesToPresent);
	}

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

void VulkanRenderer::RenderToTexture(DynamicTexturePtr texture, RenderList renderList)
{
	m_scheduler.ScheduleGpu([this, renderList = std::move(renderList),
	                         texture = std::move(texture)](CommandBuffer& commandBuffer) {
		commandBuffer.AddTexture(texture);

		const auto& textureImpl = m_device.GetImpl(*texture);

		TextureState textureState;
		textureImpl.TransitionToRenderTarget(textureState, commandBuffer);

		BuildCommandBuffer(
		    commandBuffer, renderList, textureImpl.renderPass.get(), textureImpl.framebuffer.get(), textureImpl.size);

		textureImpl.TransitionToShaderInput(textureState, commandBuffer);
	});
}

void VulkanRenderer::RenderToSurface(Surface& surface, RenderList renderList)
{
	auto& surfaceImpl = dynamic_cast<VulkanSurface&>(surface);
	if (const auto surfaceImageOpt = AddSurfaceToPresent(surfaceImpl))
	{
		const auto& surfaceImage = *surfaceImageOpt;

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
}

Task<TextureData> VulkanRenderer::CopyTextureData(TexturePtr texture)
{
	const auto& textureImpl = m_device.GetImpl(*texture);

	const TextureData textureData = {
	    .size = textureImpl.size,
	    .format = textureImpl.format,
	    .mipLevelCount = textureImpl.mipLevelCount,
	    .samples = textureImpl.samples,
	};

	const auto bufferSize = GetByteSize(textureData);

	auto task = m_scheduler.ScheduleGpu([this, texture = std::move(texture), bufferSize](CommandBuffer& commandBuffer) {
		auto buffer = CreateBufferUninitialized(
		    bufferSize, vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible,
		    m_device.GetVulkanDevice(), m_device.GetMemoryAllocator());

		const auto& textureImpl = m_device.GetImpl(*texture);

		commandBuffer.AddTexture(texture);

		TextureState textureState = {
		    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
		};
		textureImpl.TransitionToTransferSrc(textureState, commandBuffer);
		const auto extent = vk::Extent3D{textureImpl.size.width, textureImpl.size.height, 1};
		CopyImageToBuffer(
		    commandBuffer, textureImpl.image.get(), buffer.buffer.get(), textureImpl.format, extent,
		    textureImpl.mipLevelCount);
		textureImpl.TransitionToShaderInput(textureState, commandBuffer);

		return std::make_shared<VulkanBuffer>(std::move(buffer));
	});

	return m_scheduler.ScheduleAfter(task, [this, textureData](const BufferPtr& buffer) {
		const auto& data = m_device.GetImpl(*buffer).mappedData;

		TextureData ret = textureData;
		ret.pixels.resize(data.size());
		std::ranges::copy(data, ret.pixels.data());
		return ret;
	});
}

void VulkanRenderer::BuildCommandBuffer(
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
		std::vector<vk::DescriptorSet> descriptorSets;
		std::uint32_t first = 0;

		if (const auto set = GetDescriptorSet(renderList.sceneParameters.get()))
		{
			descriptorSets.push_back(set);
		}
		else
		{
			first++;
		}

		if (const auto set = GetDescriptorSet(renderList.viewParameters.get()))
		{
			descriptorSets.push_back(set);
		}

		if (!descriptorSets.empty())
		{
			commandBuffer.bindDescriptorSets(
			    vk::PipelineBindPoint::eGraphics, renderList.objects.front().pipeline->layout, first, descriptorSets, {});
		}

		for (const RenderObject& obj : renderList.objects)
		{
			commandBufferWrapper.AddParameterBlock(obj.materialParameters);

			if (obj.materialParameters)
			{
				commandBuffer.bindDescriptorSets(
				    vk::PipelineBindPoint::eGraphics, obj.pipeline->layout, 2,
				    GetDescriptorSet(obj.materialParameters.get()), {});
			}

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, obj.pipeline->pipeline.get());

			const auto& vertexBufferImpl = m_device.GetImpl(*obj.vertexBuffer);
			commandBuffer.bindVertexBuffers(0, vertexBufferImpl.buffer.get(), vk::DeviceSize{0});

			if (!obj.pushConstants.empty())
			{
				commandBuffer.pushConstants(
				    obj.pipeline->layout, vk::ShaderStageFlagBits::eVertex, 0, size32(obj.pushConstants),
				    data(obj.pushConstants));
			}

			if (obj.indexBuffer)
			{
				const auto& indexBufferImpl = m_device.GetImpl(*obj.indexBuffer);
				commandBuffer.bindIndexBuffer(indexBufferImpl.buffer.get(), vk::DeviceSize{0}, vk::IndexType::eUint16);
				commandBuffer.drawIndexed(obj.indexCount, 1, 0, 0, 0);
			}
			else
			{
				commandBuffer.draw(obj.indexCount, 1, 0, 0);
			}
		}
	}

	commandBuffer.endRenderPass();
}

std::optional<SurfaceImage> VulkanRenderer::AddSurfaceToPresent(VulkanSurface& surface)
{
	const auto lock = std::scoped_lock(m_surfacesToPresentMutex);

	if (const auto it = std::ranges::find(m_surfacesToPresent, surface.GetVulkanSurface(), &SurfaceImage::surface);
	    it != m_surfacesToPresent.end())
	{
		return *it;
	}

	if (const auto result = surface.AcquireNextImage(m_inFlightFences[m_frameNumber].get()))
	{
		return m_surfacesToPresent.emplace_back(*result);
	}

	return std::nullopt;
}

vk::DescriptorSet VulkanRenderer::GetDescriptorSet(const ParameterBlock* parameterBlock) const
{
	if (parameterBlock == nullptr)
	{
		return {};
	}
	return parameterBlock->descriptorSet[m_frameNumber % parameterBlock->descriptorSet.size()].get();
}
