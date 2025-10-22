
#include "GpuExecutor.h"

#include "Vulkan.h"

#include "Teide/Assert.h"

#include <spdlog/spdlog.h>

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
}

GpuExecutor::~GpuExecutor() noexcept
{
    spdlog::info("Destroying GpuExecutor");
    spdlog::debug("main thread: {}", GetThreadName(m_mainThread));
    spdlog::debug("this thread: {}", GetThreadName(std::this_thread::get_id()));
    TEIDE_ASSERT(m_mainThread == std::this_thread::get_id());
}

void GpuExecutor::WaitForTasks()
{
    m_queue.WaitForTasks();
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
    m_readyCommandBuffers.emplace_back();
    m_completionHandlers.emplace_back();
    return static_cast<uint32>(m_readyCommandBuffers.size() - 1);
}

void GpuExecutor::SubmitCommandBuffer(uint32 index, vk::CommandBuffer commandBuffer, OnCompleteFunction func)
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
        m_queue.Submit(commandBuffersToSubmit, std::move(callbacks));
        m_numSubmittedCommandBuffers += commandBuffersToSubmit.size();
    }
}

} // namespace Teide
