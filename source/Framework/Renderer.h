
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Internal/CommandBuffer.h"
#include "Framework/Internal/CpuExecutor.h"
#include "Framework/Internal/GpuExecutor.h"
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

	std::future<TextureData> CopyTextureData(RenderableTexturePtr texture);

private:
	struct ThreadResources
	{
		vk::UniqueCommandPool commandPool;
		std::deque<CommandBuffer> commandBuffers;
		std::uint32_t numUsedCommandBuffers = 0;
		std::uint32_t threadIndex = 0;

		void Reset(vk::Device device);
	};

	CommandBuffer& GetCommandBuffer(uint32_t threadIndex);
	void BuildCommandBuffer(
	    CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
	    vk::Framebuffer framebuffer, vk::Extent2D framebufferSize);

	static std::vector<ThreadResources> CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

	GraphicsDevice& m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
	uint32_t m_frameNumber = 0;

	CpuExecutor m_cpuExecutor;
	GpuExecutor m_gpuExecutor;

	std::mutex m_surfaceCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_surfaceCommandBuffers;
};
