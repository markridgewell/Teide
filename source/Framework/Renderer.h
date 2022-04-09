
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Internal/CommandBuffer.h"
#include "Framework/Internal/Vulkan.h"
#include "Framework/Pipeline.h"
#include "Framework/Surface.h"

#include <taskflow/taskflow.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <thread>

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

	using CompletionHandler = std::function<void()>;

	std::uint32_t AddCommandBufferSlot();
	void SubmitCommandBuffer(std::uint32_t index, vk::CommandBuffer commandBuffer, CompletionHandler function = {});

	template <typename F, typename... ArgsT>
	auto LaunchTask(F&& f, ArgsT&&... args)
	{
		return m_executor.async(std::forward<F>(f), std::forward<ArgsT>(args)...);
	}

	void WaitForTasks() { m_executor.wait_for_all(); }

	GraphicsDevice& m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	tf::Executor m_executor;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
	uint32_t m_frameNumber = 0;

	std::mutex m_schedulerMutex;
	std::jthread m_schedulerThread;
	std::stop_source m_schedulerStopSource;
	struct ScheduledTask
	{
		vk::Fence fence;
		std::function<void()> callback;
	};
	std::vector<ScheduledTask> m_scheduledTasks;

	std::mutex m_readyCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_readyCommandBuffers;
	std::unordered_map<vk::CommandBuffer, CompletionHandler> m_completionHandlers;
	std::queue<vk::UniqueFence> m_unusedSubmitFences;
	std::vector<vk::UniqueFence> m_submitFences;
	std::size_t m_numSubmittedCommandBuffers = 0;

	std::mutex m_surfaceCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_surfaceCommandBuffers;
};
