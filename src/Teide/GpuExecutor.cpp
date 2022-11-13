
#include "GpuExecutor.h"

#include "Vulkan.h"

#include <spdlog/spdlog.h>

#include <ranges>
#include <span>

namespace Teide
{

GpuExecutor::GpuExecutor(vk::Device device, vk::Queue queue) : m_device{device}, m_queue{queue}
{
    m_schedulerThread = std::jthread([this, stop_token = m_schedulerStopSource.get_token()] {
        constexpr auto timeout = std::chrono::milliseconds{2};

        std::vector<vk::Fence> fences;

        while (!stop_token.stop_requested())
        {
            fences.clear();
            {
                auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

                // Make a list of fence handles of all scheduled tasks
                std::ranges::transform(
                    m_inFlightSubmits, std::back_inserter(fences), [](const auto& s) { return s.fence.get(); });
            }

            // If there are no fences to wait on, sleep for a bit and try again
            if (fences.empty())
            {
                std::this_thread::sleep_for(timeout);
                continue;
            }

            // If there are fences, wait on them and see if any are signalled
            if (m_device.waitForFences(fences, false, Timeout(timeout)) == vk::Result::eSuccess)
            {
                auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

                // A fence was signalled, but we don't know which one yet, iterate them to find out
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
        }
    });
}

GpuExecutor::~GpuExecutor()
{
    m_schedulerStopSource.request_stop();

    auto lock = std::scoped_lock(m_readyCommandBuffersMutex);
    auto fences = std::vector<vk::Fence>();
    std::ranges::transform(m_inFlightSubmits, std::back_inserter(fences), [](const auto& s) { return s.fence.get(); });
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
    const auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

    m_readyCommandBuffers.emplace_back();
    m_completionHandlers.emplace_back();
    return static_cast<uint32>(m_readyCommandBuffers.size() - 1);
}

void GpuExecutor::SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
{
    const auto getFence = [this] {
        if (m_unusedSubmitFences.empty())
        {
            return CreateFence(m_device);
        }

        auto nextFence = std::move(m_unusedSubmitFences.front());
        m_unusedSubmitFences.pop();
        return nextFence;
    };

    const auto lock = std::scoped_lock(m_readyCommandBuffersMutex);

    commandBuffer.end();
    m_readyCommandBuffers.at(index) = commandBuffer;
    m_completionHandlers.at(index) = std::move(func);

    const auto unsubmittedCommandBuffers = std::span(m_readyCommandBuffers).subspan(m_numSubmittedCommandBuffers);
    const auto unsubmittedCompletionHandlers = std::span(m_completionHandlers).subspan(m_numSubmittedCommandBuffers);

    const auto end = std::ranges::find(unsubmittedCommandBuffers, vk::CommandBuffer{});
    const auto commandBuffersToSubmit = std::span(unsubmittedCommandBuffers.begin(), end);

    if (!commandBuffersToSubmit.empty())
    {
        auto fence = getFence();
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

} // namespace Teide
