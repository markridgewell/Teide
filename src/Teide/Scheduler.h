
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
    auto ScheduleGpu(F&& f) -> TaskForCallable<F, CommandBuffer&> // NOLINT(cppcoreguidelines-missing-std-forward)
    {
        const uint32 sequenceIndex = m_gpuExecutor.AddCommandBufferSlot();

        using FRet = std::invoke_result_t<F, CommandBuffer&>;

        auto promise = std::make_shared<std::promise<FRet>>();
        auto future = promise->get_future();

        m_cpuExecutor.LaunchTask(
            [this, sequenceIndex, f = std::forward<F>(f), promise = std::move(promise)](uint32 taskIndex) mutable {
                // TODO: This is dangerous as we're passing references to an asynchronous callback.
                // If the GPU executor is destroyed while this task is still alive it will crash.
                // Try to refactor so this isn't possible!
                CommandBuffer& commandBuffer = m_gpuExecutor.GetCommandBuffer(taskIndex);

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

    void WaitForCpu();
    void WaitForGpu();

    uint32 GetThreadCount() const { return m_cpuExecutor.GetThreadCount(); }
    uint32 GetThreadIndex() const { return m_cpuExecutor.GetThreadIndex(); }

private:
    CpuExecutor m_cpuExecutor;
    GpuExecutor m_gpuExecutor;
};

} // namespace Teide
