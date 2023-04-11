
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/VulkanConfig.h"

#include <function2/function2.hpp>

#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
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

private:
    vk::Device m_device;
    vk::Queue m_queue;

    std::mutex m_readyCommandBuffersMutex;

    std::vector<vk::CommandBuffer> m_readyCommandBuffers;
    std::vector<OnCompleteFunction> m_completionHandlers;
    std::queue<vk::UniqueFence> m_unusedSubmitFences;
    usize m_numSubmittedCommandBuffers = 0;

    struct InFlightSubmit
    {
        InFlightSubmit() = default;
        InFlightSubmit(vk::UniqueFence f, std::vector<OnCompleteFunction> c) :
            fence{std::move(f)}, callbacks{std::move(c)}
        {}

        vk::UniqueFence fence;
        std::vector<OnCompleteFunction> callbacks;
    };
    std::vector<InFlightSubmit> m_inFlightSubmits;
    std::jthread m_schedulerThread;
    std::stop_source m_schedulerStopSource;
};

} // namespace Teide
