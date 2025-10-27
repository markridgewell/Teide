
#pragma once

#include "Teide/Util/TypeHelpers.h"
#include "Teide/VulkanConfig.h"

#include <function2/function2.hpp>
#include <stdexec/execution.hpp>

#include <mutex>
#include <queue>
#include <span>
#include <thread>
#include <vector>

namespace Teide
{

namespace ex = stdexec;

class Queue
{
public:
    using CommandsRef = std::span<const vk::CommandBuffer>;

    class SubmitSender
    {
    public:
        using sender_concept = ex::sender_t;

        using completion_signatures = ex::completion_signatures<ex::set_value_t()>;

        explicit SubmitSender(CommandsRef commands, Queue& queue);

        auto connect(ex::receiver auto receiver);

    private:
        CommandsRef m_commands;
        Queue& m_queue;
    };

    using OnCompleteFunction = fu2::unique_function<void()>;

    explicit Queue(vk::Device device, vk::Queue queue);

    std::vector<vk::Fence> GetInFlightFences() const;

    void Flush();

    void Submit(std::span<const vk::CommandBuffer> commandBuffers, std::vector<OnCompleteFunction> callbacks);

    auto LazySubmit(CommandsRef commands) -> SubmitSender;

    void WaitForTasks();

private:
    vk::UniqueFence GetFence();

    template <class Receiver>
    struct SubmitOperation : Immovable
    {
        explicit SubmitOperation(CommandsRef commands, Queue& queue, Receiver receiver) :
            m_commands{commands}, m_queue{queue}, m_receiver{std::move(receiver)}
        {}

        void start() noexcept
        {
            m_queue.Submit(m_commands, {[rec = std::move(m_receiver)] { rec.set_value(); }});
        }

    private:
        CommandsRef m_commands;
        Queue& m_queue;
        Receiver m_receiver;
    };

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
