
#pragma once

#include "CommandBuffer.h"
#include "Queue.h"

#include "Teide/BasicTypes.h"
#include "Teide/Util/FrameArray.h"
#include "Teide/Util/ThreadUtils.h"
#include "Teide/VulkanConfig.h"

#include <function2/function2.hpp>

#include <deque>
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

    FrameArray<FrameResources, MaxFramesInFlight> m_frameResources;

    const std::thread::id m_mainThread = std::this_thread::get_id();

    vk::Device m_device;

    std::vector<vk::CommandBuffer> m_readyCommandBuffers;
    std::vector<OnCompleteFunction> m_completionHandlers;
    usize m_numSubmittedCommandBuffers = 0;

    Queue m_queue;
};

} // namespace Teide
