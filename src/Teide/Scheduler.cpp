
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

void Scheduler::WaitForCpu()
{
    m_cpuExecutor.WaitForTasks();
}

void Scheduler::WaitForGpu()
{
    WaitForCpu();
    m_gpuExecutor.WaitForTasks();
}

} // namespace Teide
