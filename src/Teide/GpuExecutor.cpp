
#include "GpuExecutor.h"

#include "Vulkan.h"

#include <spdlog/spdlog.h>

#include <ranges>
#include <span>

namespace Teide
{
GpuExecutor::GpuExecutor(vk::Device device, vk::Queue queue) : m_device{device}, m_queue{Queue(device, queue)}
{
    m_schedulerThread = std::thread([this] {
        constexpr auto timeout = std::chrono::milliseconds{2};

        std::vector<vk::Fence> fences;

        while (!m_schedulerStop)
        {
            const auto fencesRange = m_queue.Lock(&Queue::GetInFlightFences);
            fences.assign(fencesRange.begin(), fencesRange.end());
            // TODO C++23: Replace previous 2 lines with:
            // fences.assign_range(m_queue.Lock(&Queue::GetInFlightFences));

            if (fences.empty())
            {
                // If there are no fences to wait on, sleep for a bit and try again
                std::this_thread::sleep_for(timeout);
            }
            else
            {
                // If there are fences, wait on them and see if any are signalled
                if (m_device.waitForFences(fences, false, Timeout(timeout)) == vk::Result::eSuccess)
                {
                    m_queue.Lock(&Queue::Flush);
                }
            }
        }
    });
}

GpuExecutor::~GpuExecutor() noexcept
{
    assert(m_mainThread == std::this_thread::get_id());

    m_schedulerStop = true;
    m_schedulerThread.join();

    WaitForTasks();
}

void GpuExecutor::WaitForTasks()
{
    const auto fencesRange = m_queue.Lock(&Queue::GetInFlightFences);
    const std::vector<vk::Fence> fences(fencesRange.begin(), fencesRange.end());
    // TODO C++23: Replace previous 2 lines with:
    // const auto fences = m_queue.Lock(&Queue::GetInFlightFences) | std::ranges::to<std::vector>();

    if (!fences.empty())
    {
        constexpr auto timeout = Timeout(std::chrono::seconds{1});
        if (m_device.waitForFences(fences, true, timeout) == vk::Result::eTimeout)
        {
            spdlog::error("Timeout while waiting for command buffer execution to complete!");
        }
    }
}

uint32 GpuExecutor::AddCommandBufferSlot()
{
    return m_queue.Lock(&Queue::AddCommandBufferSlot);
}

void GpuExecutor::SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
{
    m_queue.Lock([&](Queue& queue) { queue.Submit(index, commandBuffer, std::move(func)); });
}

//---------------------------------------------------------------------------------------------------------------------

void GpuExecutor::Queue::Flush()
{
    // Iterate the fences to find out which have been signalled
    for (auto& [fence, callbacks] : m_inFlightSubmits)
    {
        if (m_device.waitForFences(fence.get(), false, 0) == vk::Result::eSuccess)
        {
            // Found one, call the attached callbacks (if any)
            for (auto& callback : std::exchange(callbacks, {}))
            {
                callback();
            }

            // Reuse the fence for later and mark the task as done
            m_device.resetFences(fence.get());
            m_unusedSubmitFences.push(std::exchange(fence, {}));
        }
    }

    // Remove done tasks
    std::erase_if(m_inFlightSubmits, [](const auto& entry) { return !entry.fence; });
}

uint32 GpuExecutor::Queue::AddCommandBufferSlot()
{
    m_readyCommandBuffers.emplace_back();
    m_completionHandlers.emplace_back();
    return static_cast<uint32>(m_readyCommandBuffers.size() - 1);
}

void GpuExecutor::Queue::Submit(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
{
    commandBuffer.end();
    m_readyCommandBuffers.at(index) = commandBuffer;
    m_completionHandlers.at(index) = std::move(func);

    const auto unsubmittedCommandBuffers = std::span(m_readyCommandBuffers).subspan(m_numSubmittedCommandBuffers);
    const auto unsubmittedCompletionHandlers = std::span(m_completionHandlers).subspan(m_numSubmittedCommandBuffers);

    const auto end = std::ranges::find(unsubmittedCommandBuffers, vk::CommandBuffer{});
    const auto commandBuffersToSubmit = std::span(unsubmittedCommandBuffers.begin(), end);

    if (!commandBuffersToSubmit.empty())
    {
        auto fence = GetFence();
        m_numSubmittedCommandBuffers += commandBuffersToSubmit.size();

        const vk::SubmitInfo submitInfo = {
            .commandBufferCount = size32(commandBuffersToSubmit),
            .pCommandBuffers = data(commandBuffersToSubmit),
        };
        m_queue.submit(submitInfo, fence.get());

        std::vector<OnCompleteFunction> callbacks;
        std::ranges::move(
            unsubmittedCompletionHandlers | std::views::filter([](const auto& x) { return x != nullptr; }),
            std::back_inserter(callbacks));
        m_inFlightSubmits.emplace_back(std::move(fence), std::move(callbacks));
    }
}

vk::UniqueFence GpuExecutor::Queue::GetFence()
{
    if (m_unusedSubmitFences.empty())
    {
        return m_device.createFenceUnique({});
    }

    auto nextFence = std::move(m_unusedSubmitFences.front());
    m_unusedSubmitFences.pop();
    return nextFence;
};

} // namespace Teide
