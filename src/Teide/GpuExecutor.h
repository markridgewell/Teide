
#pragma once

#include "CommandBuffer.h"

#include "Teide/BasicTypes.h"
#include "Teide/Util/FrameArray.h"
#include "Teide/Util/ThreadUtils.h"
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

    GpuExecutor(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex);
    ~GpuExecutor() noexcept;

    GpuExecutor(const GpuExecutor&) = delete;
    GpuExecutor(GpuExecutor&&) = delete;
    GpuExecutor& operator=(const GpuExecutor&) = delete;
    GpuExecutor& operator=(GpuExecutor&&) = delete;

    void NextFrame();

    uint32 AddCommandBufferSlot();
    CommandBuffer& GetCommandBuffer();
    void SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func = nullptr);

    void WaitForTasks();

private:
    static constexpr uint32 MaxFramesInFlight = 2;

    struct ThreadResources
    {
        vk::UniqueCommandPool commandPool;
        std::deque<CommandBuffer> commandBuffers;
        uint32 numUsedCommandBuffers = 0;
        uint32 threadIndex = 0;

        ThreadResources(uint32& i, vk::Device device, uint32 queueFamilyIndex);

        void Reset(vk::Device device);
    };

    struct FrameResources
    {
        explicit FrameResources(uint32 numThreads, vk::Device device, uint32_t queueFamilyIndex);

        ThreadMap<ThreadResources> threadResources;
    };

    class Queue
    {
    public:
        explicit Queue(vk::Device device, vk::Queue queue) : m_device{device}, m_queue{queue} {}

        std::vector<vk::Fence> GetInFlightFences() const;

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

    FrameArray<FrameResources, MaxFramesInFlight> m_frameResources;

    const std::thread::id m_mainThread = std::this_thread::get_id();

    vk::Device m_device;
    std::thread m_schedulerThread;
    std::atomic_bool m_schedulerStop = false;

    Synchronized<Queue> m_queue;
};

} // namespace Teide
