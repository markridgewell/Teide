
#pragma once

#include "Teide/VulkanConfig.h"

#include <function2/function2.hpp>

#include <mutex>
#include <queue>
#include <span>
#include <thread>
#include <vector>

namespace Teide
{

class Queue
{
public:
    using OnCompleteFunction = fu2::unique_function<void()>;

    explicit Queue(vk::Device device, vk::Queue queue);

    std::vector<vk::Fence> GetInFlightFences() const;

    void Flush();
    void Submit(std::span<const vk::CommandBuffer> commandBuffers, std::vector<OnCompleteFunction> callbacks);

    void WaitForTasks();

private:
    vk::UniqueFence GetFence();

    struct InFlightSubmit
    {
        vk::Fence GetFence() const { return fence.get(); }

        vk::UniqueFence fence;
        std::vector<OnCompleteFunction> callbacks;
    };

    vk::Device m_device;
    vk::Queue m_queue;

    mutable std::mutex m_mutex;
    std::queue<vk::UniqueFence> m_unusedSubmitFences;
    std::vector<InFlightSubmit> m_inFlightSubmits;

    // Must be the last member so it is destroyed first!
    std::jthread m_schedulerThread;
};

} // namespace Teide
