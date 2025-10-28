
#include "Queue.h"

#include "Vulkan.h"

#include "Util/ThreadUtils.h"

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ranges>
#include <stop_token>

namespace Teide
{

Queue::SubmitSender::SubmitSender(CommandsRef commands, Queue& queue) : m_commands{commands}, m_queue{queue}
{}

Queue::Queue(vk::Device device, vk::Queue queue) :
    m_device{device}, m_queue{queue}, m_schedulerThread([this](const std::stop_token& stop) {
        SetCurrentTheadName("GpuExecutor");
        constexpr auto timeout = std::chrono::milliseconds{2};

        while (!stop.stop_requested())
        {
            const auto fences = GetInFlightFences();

            if (fences.empty())
            {
                // If there are no fences to wait on, sleep for a bit and try again
                std::this_thread::sleep_for(timeout);
            }
            else
            {
                // If there are fences, wait on them and see if any are signalled
                try
                {
                    if (m_device.waitForFences(fences, false, Timeout(timeout)) == vk::Result::eSuccess)
                    {
                        Flush();
                    }
                }
                catch (const vk::DeviceLostError&)
                {
                    spdlog::critical("Device lost while waiting for fences");
                    std::abort();
                }
            }
        }

        WaitForTasks();
    })
{}

std::vector<vk::Fence> Queue::GetInFlightFences() const
{
    auto _ = std::unique_lock(m_mutex);

    return m_inFlightSubmits //
        | std::views::transform(&InFlightSubmit::GetFence)
        | std::ranges::to<std::vector>();
}

void Queue::Flush()
{
    auto _ = std::unique_lock(m_mutex);

    // Iterate the fences to find out which have been signalled
    for (auto& [fence, callback] : m_inFlightSubmits)
    {
        if (m_device.waitForFences(fence.get(), false, 0) == vk::Result::eSuccess)
        {
            // Found one, call the attached callbacks (if any)
            std::exchange(callback, {})();

            // Reuse the fence for later and mark the task as done
            m_device.resetFences(fence.get());
            m_unusedSubmitFences.push(std::exchange(fence, {}));
        }
    }

    // Remove done tasks
    std::erase_if(m_inFlightSubmits, [](const auto& entry) { return !entry.fence; });
}

void Queue::Submit(CommandsRef commands, OnCompleteFunction callback)
{
    auto _ = std::unique_lock(m_mutex);

    auto fence = GetFence();

    const vk::SubmitInfo submitInfo = {
        .commandBufferCount = size32(commands),
        .pCommandBuffers = data(commands),
    };
    m_queue.submit(submitInfo, fence.get());

    m_inFlightSubmits.emplace_back(std::move(fence), std::move(callback));
}

auto Queue::LazySubmit(CommandsRef commands) -> SubmitSender
{
    return SubmitSender(commands, *this);
}

void Queue::WaitForTasks()
{
    const auto fences = GetInFlightFences();

    if (!fences.empty())
    {
        constexpr auto timeout = std::chrono::seconds{4};
        if (m_device.waitForFences(fences, true, Timeout(timeout)) == vk::Result::eTimeout)
        {
            spdlog::error("Timeout (>{}) while waiting for command buffer execution to complete!", timeout);
        }
    }
}

vk::UniqueFence Queue::GetFence()
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
