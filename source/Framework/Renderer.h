
#pragma once

#include "Framework/BytesView.h"
#include "Framework/ForwardDeclare.h"
#include "Framework/Pipeline.h"
#include "Framework/Surface.h"
#include "Framework/Vulkan.h"

#include <taskflow/taskflow.hpp>

#include <array>
#include <cstdint>

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
	    vk::Device device, uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex,
	    uint32_t numThreads = std::thread::hardware_concurrency());

	vk::Fence BeginFrame(uint32_t frameNumber);
	void EndFrame(std::span<const SurfaceImage> images);
	void EndFrame(const SurfaceImage& image);

	void RenderToTexture(RenderableTexture& texture, RenderList renderList);
	void RenderToSurface(const SurfaceImage& surfaceImage, RenderList renderList);

private:
	struct ThreadResources
	{
		vk::UniqueCommandPool commandPool;
		vk::UniqueCommandBuffer commandBuffer;
		bool usedThisFrame = false;
		uint32_t sequenceIndex = 0;
		uint32_t threadIndex = 0;

		void Reset(vk::Device device);
	};

	vk::CommandBuffer GetCommandBuffer(uint32_t threadIndex, uint32_t sequenceIndex);
	void Render(
	    const RenderList& renderList, vk::RenderPass renderPass, vk::Framebuffer framebuffer,
	    vk::Extent2D framebufferSize, vk::CommandBuffer commandBuffer);

	static std::vector<ThreadResources> CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

	vk::Device m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;
	tf::Executor m_executor;
	tf::Taskflow m_taskflow;
	uint32_t m_nextSequenceIndex = 0;
	std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
	std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
	uint32_t m_frameNumber = 0;
};