
#pragma once

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
	GpuExecutor(vk::Device device, vk::Queue queue);
	~GpuExecutor();

	GpuExecutor(const GpuExecutor&) = delete;
	GpuExecutor(GpuExecutor&&) = delete;
	GpuExecutor& operator=(const GpuExecutor&) = delete;
	GpuExecutor& operator=(GpuExecutor&&) = delete;

	std::uint32_t AddCommandBufferSlot();
	std::shared_future<void> SubmitCommandBuffer(std::uint32_t index, vk::CommandBuffer commandBuffer);

private:
	vk::Device m_device;
	vk::Queue m_queue;

	std::mutex m_readyCommandBuffersMutex;
	std::promise<void> m_nextSubmitPromise;
	std::shared_future<void> m_nextSubmitFuture;
	std::vector<vk::CommandBuffer> m_readyCommandBuffers;
	std::queue<vk::UniqueFence> m_unusedSubmitFences;
	std::size_t m_numSubmittedCommandBuffers = 0;

	struct InFlightSubmit
	{
		vk::UniqueFence fence;
		std::promise<void> promise;
	};
	std::vector<InFlightSubmit> m_inFlightSubmits;
	std::jthread m_schedulerThread;
	std::stop_source m_schedulerStopSource;
};
