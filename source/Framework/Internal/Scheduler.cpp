
#include "Framework/Internal/Scheduler.h"

#include "Framework/Internal/Vulkan.h"

namespace
{
void SetCommandBufferDebugName(vk::UniqueCommandBuffer& commandBuffer, std::uint32_t threadIndex, std::uint32_t cbIndex)
{
	SetDebugName(commandBuffer, "RenderThread{}:CommandBuffer{}", threadIndex, cbIndex);
}

} // namespace

Scheduler::Scheduler(std::uint32_t numThreads, vk::Device device, vk::Queue queue, std::uint32_t queueFamily) :
    m_cpuExecutor(numThreads), m_gpuExecutor(device, queue), m_device{device}
{
	std::ranges::generate(m_frameResources, [=] { return CreateThreadResources(device, queueFamily, numThreads); });
}

void Scheduler::NextFrame()
{
	m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

	auto& frameResources = m_frameResources[m_frameNumber];
	for (auto& threadResources : frameResources)
	{
		threadResources.Reset(m_device);
	}
}

void Scheduler::ThreadResources::Reset(vk::Device device)
{
	device.resetCommandPool(commandPool.get());
	numUsedCommandBuffers = 0;

	// Resetting also resets the command buffers' debug names
	for (std::uint32_t i = 0; i < commandBuffers.size(); i++)
	{
		commandBuffers[i].Reset();
		SetCommandBufferDebugName(commandBuffers[i].Get(), threadIndex, i);
	}
}

CommandBuffer& Scheduler::GetCommandBuffer(uint32_t threadIndex)
{
	auto& threadResources = m_frameResources[m_frameNumber][threadIndex];

	if (threadResources.numUsedCommandBuffers == threadResources.commandBuffers.size())
	{
		const auto allocateInfo = vk::CommandBufferAllocateInfo{
		    .commandPool = threadResources.commandPool.get(),
		    .level = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = std::max(1u, static_cast<std::uint32_t>(threadResources.commandBuffers.size())),
		};

		auto newCommandBuffers = m_device.allocateCommandBuffersUnique(allocateInfo);
		const auto numCBs = static_cast<std::uint32_t>(threadResources.commandBuffers.size());
		for (std::uint32_t i = 0; i < newCommandBuffers.size(); i++)
		{
			SetCommandBufferDebugName(newCommandBuffers[i], threadIndex, i + numCBs);
			threadResources.commandBuffers.push_back(CommandBuffer(std::move(newCommandBuffers[i])));
		}
	}

	const auto commandBufferIndex = threadResources.numUsedCommandBuffers++;
	CommandBuffer& commandBuffer = threadResources.commandBuffers.at(commandBufferIndex);
	commandBuffer.Get()->begin(vk::CommandBufferBeginInfo{});

	return commandBuffer;
}

std::vector<Scheduler::ThreadResources>
Scheduler::CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads)
{
	std::vector<ThreadResources> ret;
	ret.resize(numThreads);
	int i = 0;
	std::ranges::generate(ret, [&] {
		ThreadResources res;
		res.commandPool = CreateCommandPool(queueFamilyIndex, device);
		SetDebugName(res.commandPool, "RenderThread{}:CommandPool", i);
		res.threadIndex = i;
		i++;
		return res;
	});

	return ret;
}
