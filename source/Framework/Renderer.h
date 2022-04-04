
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Pipeline.h"
#include "Framework/Surface.h"
#include "Framework/Vulkan.h"

#include <taskflow/taskflow.hpp>

#include <array>
#include <cstdint>
#include <deque>

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

template <class T>
class Future
{
public:
	const T& Get() { return m_value; }

private:
	T m_value;
};

class Renderer
{
public:
	explicit Renderer(
	    GraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex,
	    uint32_t numThreads = std::thread::hardware_concurrency());

	std::uint32_t GetFrameNumber() const;
	vk::Fence BeginFrame();
	void EndFrame(std::span<const SurfaceImage> images);
	void EndFrame(const SurfaceImage& image);

	void RenderToTexture(RenderableTexture& texture, RenderList renderList);
	void RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList);

	Future<TextureData> CopyTextureData(RenderableTexture& texture);

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
	void Render(
	    const RenderList& renderList, vk::RenderPass renderPass, vk::Framebuffer framebuffer,
	    vk::Extent2D framebufferSize, vk::CommandBuffer commandBuffer);

	static std::vector<ThreadResources> CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

	std::uint32_t AddCommandBufferSlot();
	void SubmitCommandBuffer(std::uint32_t index, vk::CommandBuffer commandBuffer);

	template <typename F, typename... ArgsT>
	auto LaunchTask(F&& f, ArgsT&&... args)
	{
		m_tasks.push_back(m_executor.async(std::forward<F>(f), std::forward<ArgsT>(args)...));
	}

	void WaitForTasks()
	{
		for (auto& task : m_tasks)
		{
			task.wait();
		}
		m_tasks.clear();
	}

	GraphicsDevice& m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	tf::Executor m_executor;
	std::vector<tf::Future<void>> m_tasks;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
	uint32_t m_frameNumber = 0;

	std::mutex m_readyCommandBuffersMutex;
	std::vector<vk::CommandBuffer> m_readyCommandBuffers;
	// std::size_t m_numSubmittedCommandBuffers = 0;
};
