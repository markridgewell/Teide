
#pragma once

#include "Framework/Internal/CommandBuffer.h"
#include "Framework/Internal/CpuExecutor.h"
#include "Framework/Internal/GpuExecutor.h"

#include <vector>

template <class T = void>
using Task = std::shared_future<T>;

class Scheduler
{
public:
	Scheduler(std::uint32_t numThreads, vk::Device device, vk::Queue queue, std::uint32_t queueFamily);

	void NextFrame();

	template <std::invocable<> F>
	auto Schedule(F&& f) -> Task<>
	{
		return m_cpuExecutor.LaunchTask(std::forward<F>(f));
	}

	template <std::invocable<uint32_t> F>
	auto Schedule(F&& f) -> Task<>
	{
		return m_cpuExecutor.LaunchTask(std::forward<F>(f));
	}

	template <std::invocable<CommandBuffer&> F>
	auto Schedule(F&& f) -> Task<>
	{
		const std::uint32_t sequenceIndex = m_gpuExecutor.AddCommandBufferSlot();

		return m_cpuExecutor.LaunchTask([this, sequenceIndex, f = std::forward<F>(f)](std::uint32_t taskIndex) mutable {
			CommandBuffer& commandBuffer = GetCommandBuffer(taskIndex);
			std::forward<F>(f)(commandBuffer);
			m_gpuExecutor.SubmitCommandBuffer(sequenceIndex, commandBuffer);
		});
	}

	template <std::invocable<> F>
	auto Schedule(F&& f, Task<> dependency) -> Task<>
	{
		return m_cpuExecutor.LaunchTask(std::forward<F>(f), std::move(dependency));
	}

	void WaitForTasks() { m_cpuExecutor.WaitForTasks(); }

	// private:
	static constexpr uint32_t MaxFramesInFlight = 2;

	struct ThreadResources
	{
		vk::UniqueCommandPool commandPool;
		std::deque<CommandBuffer> commandBuffers;
		std::uint32_t numUsedCommandBuffers = 0;
		std::uint32_t threadIndex = 0;

		void Reset(vk::Device device);
	};

	static std::vector<ThreadResources> CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	CommandBuffer& GetCommandBuffer(uint32_t threadIndex);

	CpuExecutor m_cpuExecutor;
	GpuExecutor m_gpuExecutor;

	vk::Device m_device;
	std::uint32_t m_frameNumber = 0;

	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
};
