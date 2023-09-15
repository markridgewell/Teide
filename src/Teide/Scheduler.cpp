
#include "Scheduler.h"

#include "Vulkan.h"

namespace Teide
{

Scheduler::Scheduler(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex) :
    m_cpuExecutor(numThreads), m_gpuExecutor(numThreads, device, queue, queueFamilyIndex)
{}

void Scheduler::NextFrame()
{
    m_gpuExecutor.NextFrame();
}

CommandBuffer& Scheduler::GetCommandBuffer(uint32 threadIndex)
{
    return m_gpuExecutor.GetCommandBuffer(threadIndex);
}

} // namespace Teide
