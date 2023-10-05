
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Synchronized.h"
#include "Teide/VulkanConfig.h"

#include <function2/function2.hpp>

#include <atomic>
#include <future>
#include <queue>
#include <ranges>
#include <thread>
#include <vector>

namespace Teide
{
class GpuExecutor
{
public:
    using OnCompleteFunction = fu2::unique_function<void()>;

    GpuExecutor(vk::Device device, vk::Queue queue);
    ~GpuExecutor() noexcept;

    GpuExecutor(const GpuExecutor&) = delete;
    GpuExecutor(GpuExecutor&&) = delete;
    GpuExecutor& operator=(const GpuExecutor&) = delete;
    GpuExecutor& operator=(GpuExecutor&&) = delete;

    uint32 AddCommandBufferSlot();
    void SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func = nullptr);

    void WaitForTasks();

private:
    class Queue
    {
    public:
        explicit Queue(vk::Device device, vk::Queue queue) : m_device{device}, m_queue{queue} {}

        auto GetInFlightFences() const { return m_inFlightSubmits | std::views::transform(&InFlightSubmit::GetFence); }

        void Flush();
        uint32 AddCommandBufferSlot();
        void Submit(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func);

    private:
        vk::UniqueFence GetFence();

        struct InFlightSubmit
        {
            InFlightSubmit() = default;
            InFlightSubmit(vk::UniqueFence f, std::vector<OnCompleteFunction> c) :
                fence{std::move(f)}, callbacks{std::move(c)}
            {}

            vk::Fence GetFence() const { return fence.get(); }

            vk::UniqueFence fence;
            std::vector<OnCompleteFunction> callbacks;
        };

        vk::Device m_device;
        vk::Queue m_queue;

        std::vector<vk::CommandBuffer> m_readyCommandBuffers;
        std::vector<OnCompleteFunction> m_completionHandlers;
        std::queue<vk::UniqueFence> m_unusedSubmitFences;
        usize m_numSubmittedCommandBuffers = 0;

        std::vector<InFlightSubmit> m_inFlightSubmits;
    };

    const std::thread::id m_mainThread = std::this_thread::get_id();

    vk::Device m_device;
    std::thread m_schedulerThread;
    std::atomic_bool m_schedulerStop = false;

    Synchronized<Queue> m_queue;
};

} // namespace Teide
