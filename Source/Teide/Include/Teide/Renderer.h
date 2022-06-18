
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Internal/CommandBuffer.h"
#include "Framework/Internal/Scheduler.h"
#include "Framework/Internal/Vulkan.h"
#include "Framework/Pipeline.h"
#include "Framework/Surface.h"

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>

class GraphicsDevice;

struct RenderObject
{
	BufferPtr vertexBuffer;
	BufferPtr indexBuffer;
	uint32_t indexCount = 0;
	PipelinePtr pipeline;
	ParameterBlockPtr materialParameters;
	BytesView pushConstants;
};

struct RenderList
{
	std::vector<vk::ClearValue> clearValues;

	ParameterBlockPtr sceneParameters;
	ParameterBlockPtr viewParameters;

	std::vector<RenderObject> objects;
};

class Renderer
{
public:
	explicit Renderer(
	    GraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex,
	    uint32_t numThreads = std::thread::hardware_concurrency());

	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	std::uint32_t GetFrameNumber() const;
	vk::Fence BeginFrame();
	void EndFrame(std::span<const SurfaceImage> images);
	void EndFrame(const SurfaceImage& image);

	void RenderToTexture(RenderableTexturePtr texture, RenderList renderList);
	void RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList);

	Task<TextureData> CopyTextureData(RenderableTexturePtr texture);

private:
	void BuildCommandBuffer(
	    CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
	    vk::Framebuffer framebuffer, vk::Extent2D framebufferSize);

	vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

	GraphicsDevice& m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	uint32_t m_frameNumber = 0;

	Scheduler m_scheduler;

	std::mutex m_surfaceCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_surfaceCommandBuffers;
};
