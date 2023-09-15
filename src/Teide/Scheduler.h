
#pragma once

#include "CommandBuffer.h"
#include "CpuExecutor.h"
#include "GpuExecutor.h"

#include <vector>

namespace Teide
{

class Scheduler
{
public:
    Scheduler(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex);

    void NextFrame();

    template <std::invocable<> F>
    auto Schedule(F&& f) -> TaskForCallable<F>
    {
        return m_cpuExecutor.LaunchTask(std::forward<F>(f));
    }

    template <std::invocable<uint32> F>
    auto Schedule(F&& f) -> TaskForCallable<F, uint32>
    {
        return m_cpuExecutor.LaunchTask(std::forward<F>(f));
    }

    template <std::invocable<CommandBuffer&> F>
    auto ScheduleGpu(F&& f) -> TaskForCallable<F, CommandBuffer&>
    {
        const uint32 sequenceIndex = m_gpuExecutor.AddCommandBufferSlot();

        using FRet = std::invoke_result_t<F, CommandBuffer&>;

        auto promise = std::make_shared<std::promise<FRet>>();
        auto future = promise->get_future();

        m_cpuExecutor.LaunchTask(
            [this, sequenceIndex, f = std::forward<F>(f), promise = std::move(promise)](uint32 taskIndex) mutable {
                CommandBuffer& commandBuffer = GetCommandBuffer(taskIndex);

                if constexpr (std::is_void_v<FRet>)
                {
                    std::forward<F>(f)(commandBuffer); // call the callback
                    auto callback = [promise = std::move(promise)]() mutable { promise->set_value(); };
                    m_gpuExecutor.SubmitCommandBuffer(sequenceIndex, commandBuffer, std::move(callback));
                }
                else
                {
                    FRet ret = std::forward<F>(f)(commandBuffer); // call the callback
                    auto callback = [promise = std::move(promise), ret = std::move(ret)]() mutable {
                        promise->set_value(std::move(ret));
                    };
                    m_gpuExecutor.SubmitCommandBuffer(sequenceIndex, commandBuffer, std::move(callback));
                }
            });

        return future;
    }

    template <std::invocable<> F>
    auto ScheduleAfter(Task<> dependency, F&& f) -> TaskForCallable<F>
    {
        return m_cpuExecutor.LaunchTask(std::forward<F>(f), std::move(dependency));
    }

    template <class T, std::invocable<T> F>
    auto ScheduleAfter(Task<T> dependency, F&& f) -> TaskForCallable<F, T>
    {
        return m_cpuExecutor.LaunchTask(std::forward<F>(f), std::move(dependency));
    }

    void WaitForTasks() { m_cpuExecutor.WaitForTasks(); }

    uint32 GetThreadIndex() const { return m_cpuExecutor.GetThreadIndex(); }

    CommandBuffer& GetCommandBuffer(uint32 threadIndex);

private:
    CpuExecutor m_cpuExecutor;
    GpuExecutor m_gpuExecutor;
};

} // namespace Teide
