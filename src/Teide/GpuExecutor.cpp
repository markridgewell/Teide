
#include "GpuExecutor.h"

#include "Vulkan.h"

#include <spdlog/spdlog.h>

#include <ranges>
#include <span>

namespace Teide
{

namespace
{
    void SetCommandBufferDebugName(vk::UniqueCommandBuffer& commandBuffer, uint32 threadIndex, uint32 cbIndex)
    {
        SetDebugName(commandBuffer, "RenderThread{}:CommandBuffer{}", threadIndex, cbIndex);
    }

} // namespace

GpuExecutor::GpuExecutor(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex) :
    m_device{device}, m_queue{queue}
{
    std::ranges::generate(m_frameResources, [&]() {
        std::vector<ThreadResources> ret;
        ret.resize(numThreads);
        uint32 i = 0;
        std::ranges::generate(ret, [&] {
            ThreadResources res;
            res.commandPool = CreateCommandPool(queueFamilyIndex, device);
            SetDebugName(res.commandPool, "RenderThread{}:CommandPool", i);
            res.threadIndex = i;
            i++;
            return res;
        });

        return ret;
    });

    m_schedulerThread = std::thread([this] {
        constexpr auto timeout = std::chrono::milliseconds{2};

        std::vector<vk::Fence> fences;

        while (!m_schedulerStop)
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

GpuExecutor::~GpuExecutor() noexcept
{
    m_schedulerStop = true;
    m_schedulerThread.join();

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

void GpuExecutor::NextFrame()
{
    m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

    auto& frameResources = m_frameResources[m_frameNumber];
    for (auto& threadResources : frameResources)
    {
        threadResources.Reset(m_device);
    }
}

void GpuExecutor::ThreadResources::Reset(vk::Device device)
{
    device.resetCommandPool(commandPool.get());
    numUsedCommandBuffers = 0;

    // Resetting also resets the command buffers' debug names
    for (uint32 i = 0; i < commandBuffers.size(); i++)
    {
        commandBuffers[i].Reset();
        SetCommandBufferDebugName(commandBuffers[i].Get(), threadIndex, i);
    }
}

CommandBuffer& GpuExecutor::GetCommandBuffer(uint32 threadIndex)
{
    auto& threadResources = m_frameResources[m_frameNumber][threadIndex];

    if (threadResources.numUsedCommandBuffers == threadResources.commandBuffers.size())
    {
        const vk::CommandBufferAllocateInfo allocateInfo = {
            .commandPool = threadResources.commandPool.get(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = std::max(1u, static_cast<uint32>(threadResources.commandBuffers.size())),
        };

        auto newCommandBuffers = m_device.allocateCommandBuffersUnique(allocateInfo);
        const auto numCBs = static_cast<uint32>(threadResources.commandBuffers.size());
        for (uint32 i = 0; i < newCommandBuffers.size(); i++)
        {
            SetCommandBufferDebugName(newCommandBuffers[i], threadIndex, i + numCBs);
            threadResources.commandBuffers.emplace_back(std::move(newCommandBuffers[i]));
        }
    }

    const auto commandBufferIndex = threadResources.numUsedCommandBuffers++;
    CommandBuffer& commandBuffer = threadResources.commandBuffers.at(commandBufferIndex);
    commandBuffer.Get()->begin(vk::CommandBufferBeginInfo{});

    return commandBuffer;
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
            return m_device.createFenceUnique({});
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
