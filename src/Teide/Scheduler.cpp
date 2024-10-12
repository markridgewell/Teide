
#include <sal.h>
#undef __valid

#include "Scheduler.h"
#include "Vulkan.h"

#include <unifex/scheduler_concepts.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>

template <typename Scheduler, typename F>
auto run_on(Scheduler&& s, F&& func)
{
    return unifex::then(unifex::schedule((Scheduler&&)s), (F&&)func);
}

namespace Teide
{

Scheduler::Scheduler(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex) :
    m_cpuExecutor(numThreads), m_threadPool(numThreads), m_gpuExecutor(numThreads, device, queue, queueFamilyIndex)
{
    using namespace unifex;
    auto tp = m_threadPool.get_scheduler();
    std::atomic<int> x = 0;

    sync_wait(
        when_all(unifex::schedule(tp) | unifex::then([&] { ++x; }), run_on(tp, [&] { ++x; }), run_on(tp, [&] { ++x; })));

    UNIFEX_ASSERT(x == 3);
}

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
