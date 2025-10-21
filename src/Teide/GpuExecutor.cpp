
#include "GpuExecutor.h"

#include "Vulkan.h"

#include "Teide/Assert.h"

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ranges>
#include <span>
#include <sstream>

namespace Teide
{

namespace
{
    void SetCommandBufferDebugName(vk::UniqueCommandBuffer& commandBuffer, uint32 threadIndex, uint32 cbIndex)
    {
        SetDebugName(commandBuffer, "RenderThread{}:CommandBuffer{}", threadIndex, cbIndex);
    }

    std::string GetThreadName(std::thread::id id)
    {
        std::stringstream ss;
        ss << id;
        return ss.str();
    }

} // namespace

GpuExecutor::GpuExecutor(uint32 numThreads, vk::Device device, vk::Queue queue, uint32 queueFamilyIndex) :
    m_frameResources(numThreads, device, queueFamilyIndex), m_device{device}, m_queue{Queue(device, queue)}
{
    spdlog::info("Creating GpuExecutor");
    spdlog::debug("this thread: {}", GetThreadName(std::this_thread::get_id()));
    m_schedulerThread = std::thread([this] {
        SetCurrentTheadName("GpuExecutor");
        constexpr auto timeout = std::chrono::milliseconds{2};

        while (!m_schedulerStop)
        {
            const auto fences = m_queue.Lock(&Queue::GetInFlightFences);

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
                        m_queue.Lock(&Queue::Flush);
                    }
                }
                catch (const vk::DeviceLostError&)
                {
                    spdlog::critical("Device lost while waiting for fences");
                    std::abort();
                }
            }
        }
    });
}

GpuExecutor::~GpuExecutor() noexcept
{
    spdlog::info("Destroying GpuExecutor");
    spdlog::debug("main thread: {}", GetThreadName(m_mainThread));
    spdlog::debug("this thread: {}", GetThreadName(std::this_thread::get_id()));
    TEIDE_ASSERT(m_mainThread == std::this_thread::get_id());

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
        constexpr auto timeout = std::chrono::seconds{4};
        if (m_device.waitForFences(fences, true, Timeout(timeout)) == vk::Result::eTimeout)
        {
            spdlog::error("Timeout (>{}) while waiting for command buffer execution to complete!", timeout);
        }
    }
}

void GpuExecutor::NextFrame()
{
    m_frameResources.NextFrame();

    m_frameResources.Current().threadResources.LockAll([this](auto& threadResources) { threadResources.Reset(m_device); });
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

GpuExecutor::FrameResources::FrameResources(uint32 numThreads, vk::Device device, uint32_t queueFamilyIndex) :
    threadResources{numThreads, [&, i = 0u]() mutable { return ThreadResources(i, device, queueFamilyIndex); }}
{}

GpuExecutor::ThreadResources::ThreadResources(uint32& i, vk::Device device, uint32 queueFamilyIndex) :
    commandPool{CreateCommandPool(queueFamilyIndex, device)}, threadIndex(i)
{
    SetDebugName(commandPool, "RenderThread{}:CommandPool", i);

    i++;
}

CommandBuffer& GpuExecutor::GetCommandBuffer()
{
    return m_frameResources.Current().threadResources.LockCurrent([this](auto& threadResources) -> CommandBuffer& {
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
                SetCommandBufferDebugName(newCommandBuffers[i], threadResources.threadIndex, i + numCBs);
                threadResources.commandBuffers.emplace_back(std::move(newCommandBuffers[i]));
            }
        }

        const auto commandBufferIndex = threadResources.numUsedCommandBuffers++;
        CommandBuffer& commandBuffer = threadResources.commandBuffers.at(commandBufferIndex);
        commandBuffer.Get()->begin(vk::CommandBufferBeginInfo{});

        return commandBuffer;
    });
}

uint32 GpuExecutor::AddCommandBufferSlot()
{
    return m_queue.Lock(&Queue::AddCommandBufferSlot);
}

void GpuExecutor::SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
{
    m_queue.Lock(&Queue::SubmitCommandBuffer, index, commandBuffer, std::move(func));
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<vk::Fence> GpuExecutor::Queue::GetInFlightFences() const
{
    const auto range = m_inFlightSubmits | std::views::transform(&InFlightSubmit::GetFence);
    return {range.begin(), range.end()};
}

void GpuExecutor::Queue::Flush()
{
    // Iterate the fences to find out which have been signalled
    for (auto& [fence, callbacks] : m_inFlightSubmits)
    {
        if (m_device.waitForFences(fence.get(), false, 0) == vk::Result::eSuccess)
        {
            // Found one, call the attached callbacks (if any)
            for (auto&& callback : std::exchange(callbacks, {}))
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

void GpuExecutor::Queue::SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
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
        std::vector<OnCompleteFunction> callbacks;
        std::ranges::move(
            unsubmittedCompletionHandlers | std::views::filter([](const auto& x) { return x != nullptr; }),
            std::back_inserter(callbacks));
        Submit(commandBuffersToSubmit, std::move(callbacks));
        m_numSubmittedCommandBuffers += commandBuffersToSubmit.size();
    }
}

void GpuExecutor::Queue::Submit(std::span<const vk::CommandBuffer> commandBuffers, std::vector<OnCompleteFunction> callbacks)
{
    auto fence = GetFence();

    const vk::SubmitInfo submitInfo = {
        .commandBufferCount = size32(commandBuffers),
        .pCommandBuffers = data(commandBuffers),
    };
    m_queue.submit(submitInfo, fence.get());

    m_inFlightSubmits.emplace_back(std::move(fence), std::move(callbacks));
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
