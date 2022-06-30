
#pragma once

#include "CommandBuffer.h"
#include "Scheduler.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"
#include "Vulkan.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanSurface.h"

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>

class VulkanRenderer : public Renderer
{
public:
	explicit VulkanRenderer(
	    VulkanGraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex,
	    uint32_t numThreads = std::thread::hardware_concurrency());

	~VulkanRenderer();

	std::uint32_t GetFrameNumber() const override;
	void BeginFrame() override;
	void EndFrame() override;

	void RenderToTexture(DynamicTexturePtr texture, RenderList renderList) override;
	void RenderToSurface(Surface& surface, RenderList renderList) override;

	Task<TextureData> CopyTextureData(TexturePtr texture) override;

private:
	void BuildCommandBuffer(
	    CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
	    vk::Framebuffer framebuffer, vk::Extent2D framebufferSize);

	std::optional<SurfaceImage> AddSurfaceToPresent(VulkanSurface& surface);

	vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

	VulkanGraphicsDevice& m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	uint32_t m_frameNumber = 0;

	Scheduler m_scheduler;

	std::mutex m_surfaceCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_surfaceCommandBuffers;

	std::mutex m_surfacesToPresentMutex;
	std::vector<SurfaceImage> m_surfacesToPresent;
};
