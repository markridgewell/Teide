
#include "VulkanRenderer.h"

#include "Teide/Renderer.h"
#include "Types/TextureData.h"
#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanParameterBlock.h"
#include "VulkanPipeline.h"
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
    ShaderPtr shaderEnvironment) :
    m_device{device},
    m_graphicsQueue{device.GetVulkanDevice().getQueue(graphicsFamilyIndex, 0)},
    m_shaderEnvironment{std::move(shaderEnvironment)}
{
	const auto vkdevice = device.GetVulkanDevice();

	if (presentFamilyIndex)
	{
		m_presentQueue = vkdevice.getQueue(*presentFamilyIndex, 0);
	}

	std::ranges::generate(m_renderFinished, [=] { return CreateSemaphore(vkdevice); });
	std::ranges::generate(m_inFlightFences, [=] { return CreateFence(vkdevice, vk::FenceCreateFlagBits::eSignaled); });

	const auto numThreads = device.GetScheduler().GetThreadCount();
	std::ranges::generate(
	    m_frameResources, [=] { return FrameResources{.threadResources = std::vector<ThreadResources>(numThreads)}; });
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

void VulkanRenderer::BeginFrame(const ShaderParameters& sceneParameters)
{
	constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

	m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

	[[maybe_unused]] const auto waitResult
	    = m_device.GetVulkanDevice().waitForFences(m_inFlightFences[m_frameNumber].get(), true, timeout);
	assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

	m_device.GetScheduler().NextFrame();

	auto& frameResources = m_frameResources[m_frameNumber];
	const auto pblockData = ParameterBlockData{
	    .shader = m_shaderEnvironment,
	    .blockType = ParameterBlockType::Scene,
	    .parameters = sceneParameters, // COPY!
	};
	frameResources.sceneParameters = m_device.CreateParameterBlock(pblockData, "Scene");
	for (auto& threadResources : frameResources.threadResources)
	{
		threadResources.viewParameters.clear();
	}
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

	m_device.GetScheduler().WaitForTasks();

	device.resetFences(fenceToSignal);

	// Submit the surface command buffer(s)
	{
		std::vector<vk::CommandBuffer> commandBuffers;
		{
			const auto lock = std::scoped_lock(m_surfaceCommandBuffersMutex);
			std::swap(m_surfaceCommandBuffers, commandBuffers);
		}

		for (const auto& surfaceImage : images)
		{
			commandBuffers.push_back(surfaceImage.prePresentCommandBuffer);
		}

		const auto submitInfo = vk::SubmitInfo{
		    .waitSemaphoreCount = size32(waitSemaphores),
		    .pWaitSemaphores = data(waitSemaphores),
		    .pWaitDstStageMask = data(waitStages),
		    .commandBufferCount = size32(commandBuffers),
		    .pCommandBuffers = data(commandBuffers),
		    .signalSemaphoreCount = size32(signalSemaphores),
		    .pSignalSemaphores = data(signalSemaphores),
		};
		m_graphicsQueue.submit(submitInfo, fenceToSignal);
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
	const bool isColorTarget = !HasDepthOrStencilComponent(texture->GetFormat());
	const auto framebufferLayout = FramebufferLayout{
	    .colorFormat = isColorTarget ? texture->GetFormat() : vk::Format::eUndefined,
	    .depthStencilFormat = isColorTarget ? vk::Format::eUndefined : texture->GetFormat(),
	    .sampleCount = texture->GetSampleCount(),
	};

	ScheduleGpu([this, renderList = std::move(renderList), texture = std::move(texture),
	             framebufferLayout](CommandBuffer& commandBuffer) {
		commandBuffer.AddTexture(texture);

		const auto& textureImpl = m_device.GetImpl(*texture);

		TextureState textureState;
		textureImpl.TransitionToRenderTarget(textureState, commandBuffer);

		BuildCommandBuffer(commandBuffer, renderList, framebufferLayout, {}, textureImpl.size, {textureImpl.imageView.get()});

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

		const auto framebufferLayout = FramebufferLayout{
		    .colorFormat = surface.GetColorFormat(),
		    .depthStencilFormat = surface.GetDepthFormat(),
		    .sampleCount = surface.GetSampleCount(),
		};

		m_device.GetScheduler().Schedule([=, this, renderList = std::move(renderList)](uint32_t taskIndex) {
			CommandBuffer& commandBuffer = m_device.GetScheduler().GetCommandBuffer(taskIndex);

			BuildCommandBuffer(commandBuffer, renderList, framebufferLayout, framebuffer, extent, {});

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
	    .sampleCount = textureImpl.sampleCount,
	};

	const auto bufferSize = GetByteSize(textureData);

	auto task = ScheduleGpu([this, texture = std::move(texture), bufferSize](CommandBuffer& commandBuffer) {
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

	return m_device.GetScheduler().ScheduleAfter(task, [this, textureData](const BufferPtr& buffer) {
		const auto& data = m_device.GetImpl(*buffer).mappedData;

		TextureData ret = textureData;
		ret.pixels.resize(data.size());
		std::ranges::copy(data, ret.pixels.data());
		return ret;
	});
}

void VulkanRenderer::BuildCommandBuffer(
    CommandBuffer& commandBufferWrapper, const RenderList& renderList, const FramebufferLayout& framebufferLayout,
    vk::Framebuffer framebuffer, vk::Extent2D framebufferSize, std::vector<vk::ImageView> framebufferAttachments)
{
	using std::data;

	vk::CommandBuffer commandBuffer = commandBufferWrapper;

	const auto renderPassInfo = RenderPassInfo{
	    .colorLoadOp = renderList.clearColorValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
	    .colorStoreOp = vk::AttachmentStoreOp::eStore,
	    .depthLoadOp = renderList.clearDepthValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
	    .depthStoreOp = vk::AttachmentStoreOp::eStore,
	    .stencilLoadOp = renderList.clearStencilValue ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
	    .stencilStoreOp = vk::AttachmentStoreOp::eStore,
	};

	const auto renderPass = m_device.CreateRenderPass(framebufferLayout, renderPassInfo);

	if (!framebuffer)
	{
		framebuffer = m_device.CreateFramebuffer(renderPass, framebufferSize, framebufferAttachments);
	}

	auto clearValues = std::vector<vk::ClearValue>();
	if (framebufferLayout.colorFormat != vk::Format::eUndefined)
	{
		if (renderList.clearColorValue)
		{
			clearValues.push_back(vk::ClearColorValue{*renderList.clearColorValue});
		}
		else
		{
			clearValues.emplace_back();
		}
	}
	if (framebufferLayout.depthStencilFormat != vk::Format::eUndefined)
	{
		clearValues.push_back(vk::ClearDepthStencilValue{
		    renderList.clearDepthValue.value_or(1.0f), renderList.clearStencilValue.value_or(0)});
	};

	const auto renderPassBegin = vk::RenderPassBeginInfo{
	    .renderPass = renderPass,
	    .framebuffer = framebuffer,
	    .renderArea = {.offset = {0, 0}, .extent = framebufferSize},
	    .clearValueCount = size32(clearValues),
	    .pClearValues = data(clearValues),
	};

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

	const auto threadIndex = m_device.GetScheduler().GetThreadIndex();
	const auto viewParamsData = ParameterBlockData{
	    .shader = m_shaderEnvironment,
	    .blockType = ParameterBlockType::View,
	    .parameters = renderList.viewParameters, // COPY!
	};
	const auto viewParamsName = fmt::format("{}:View", renderList.name);
	const auto viewParameters = AddViewParameterBlock(
	    threadIndex,
	    m_device.CreateParameterBlock(viewParamsData, viewParamsName.c_str(), commandBufferWrapper, threadIndex));

	commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

	if (!renderList.objects.empty())
	{
		std::vector<vk::DescriptorSet> descriptorSets;
		std::uint32_t first = 0;

		if (const auto set = GetDescriptorSet(GetSceneParameterBlock().get()))
		{
			descriptorSets.push_back(set);
		}
		else
		{
			first++;
		}

		if (const auto set = GetDescriptorSet(viewParameters.get()))
		{
			descriptorSets.push_back(set);
		}

		if (!descriptorSets.empty())
		{
			const auto& firstPipeline = m_device.GetImpl(*renderList.objects.front().pipeline);
			commandBuffer.bindDescriptorSets(
			    vk::PipelineBindPoint::eGraphics, firstPipeline.layout, first, descriptorSets, {});
		}

		for (const RenderObject& obj : renderList.objects)
		{
			const auto& pipeline = m_device.GetImpl(*obj.pipeline);

			commandBufferWrapper.AddParameterBlock(obj.materialParameters);

			if (obj.materialParameters)
			{
				commandBuffer.bindDescriptorSets(
				    vk::PipelineBindPoint::eGraphics, pipeline.layout, 2,
				    GetDescriptorSet(obj.materialParameters.get()), {});
			}

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline.get());

			const auto& vertexBufferImpl = m_device.GetImpl(*obj.vertexBuffer);
			commandBuffer.bindVertexBuffers(0, vertexBufferImpl.buffer.get(), vk::DeviceSize{0});

			if (!obj.pushConstants.empty())
			{
				commandBuffer.pushConstants(
				    pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, size32(obj.pushConstants), data(obj.pushConstants));
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
	const auto& parameterBlockImpl = m_device.GetImpl(*parameterBlock);
	return parameterBlockImpl.descriptorSet.get();
}
