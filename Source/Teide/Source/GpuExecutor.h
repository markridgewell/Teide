
#pragma once

#include <function2/function2.hpp>
#include <vulkan/vulkan.hpp>

#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

class GpuExecutor
{
public:
	using OnCompleteFunction = fu2::unique_function<void()>;

	GpuExecutor(vk::Device device, vk::Queue queue);
	~GpuExecutor();

	GpuExecutor(const GpuExecutor&) = delete;
	GpuExecutor(GpuExecutor&&) = delete;
	GpuExecutor& operator=(const GpuExecutor&) = delete;
	GpuExecutor& operator=(GpuExecutor&&) = delete;

	std::uint32_t AddCommandBufferSlot();
	void SubmitCommandBuffer(std::uint32_t index, vk::CommandBuffer commandBuffer, OnCompleteFunction func = nullptr);

private:
	vk::Device m_device;
	vk::Queue m_queue;

	std::mutex m_readyCommandBuffersMutex;

	std::vector<vk::CommandBuffer> m_readyCommandBuffers;
	std::vector<OnCompleteFunction> m_completionHandlers;
	std::queue<vk::UniqueFence> m_unusedSubmitFences;
	std::size_t m_numSubmittedCommandBuffers = 0;

	struct InFlightSubmit
	{
		vk::UniqueFence fence;
		std::vector<OnCompleteFunction> callbacks;
	};
	std::vector<InFlightSubmit> m_inFlightSubmits;
	std::jthread m_schedulerThread;
	std::stop_source m_schedulerStopSource;
};
