
#pragma once

#include "Teide/Assert.h"
#include "Teide/Util/ThreadUtils.h"
#include "Teide/Util/TypeHelpers.h"

#include <stdexec/execution.hpp>

#include <functional>
#include <variant>

namespace ex = stdexec;

namespace Teide::detail
{

template <class T>
using CompletionSignaturesFor = stdexec::completion_signatures<stdexec::set_value_t(T), stdexec::set_stopped_t()>;

template <class T>
struct CompletionState : MoveOnly
{
    using Sigs = CompletionSignaturesFor<T>;

    struct AwaitingResult
    {};

    struct OnResult
    {
        std::function<void(T)> setValue;
        std::function<void()> setStopped;
    };

    using ResultValue = T;

    struct Canceled
    {};

    using State = std::variant<AwaitingResult, OnResult, ResultValue, Canceled>;

    auto SetReceiver(ex::receiver_of<Sigs> auto& receiver) -> bool
    {
        return state.Lock([&receiver](State& state) {
            TEIDE_ASSERT(!std::holds_alternative<OnResult>(state));

            if (std::holds_alternative<Canceled>(state))
            {
                receiver.set_stopped();
                return true;
            }

            if (auto* result = std::get_if<ResultValue>(&state))
            {
                receiver.set_value(std::move(*result));
                return true;
            }

            if (std::holds_alternative<AwaitingResult>(state))
            {
                state = OnResult{
                    .setValue = [&receiver](T value) { receiver.set_value(std::move(value)); },
                    .setStopped = [&receiver] { receiver.set_stopped(); },
                };
            }

            return false;
        });
    }

    void SetValue(T value)
    {
        state.Lock([&value](State& state) {
            TEIDE_ASSERT(!std::holds_alternative<ResultValue>(state));

            if (std::holds_alternative<Canceled>(state))
            {
                // nothing to do
                return;
            }

            if (auto* onResult = std::get_if<OnResult>(&state))
            {
                onResult->setValue(std::move(value));
            }

            if (std::holds_alternative<AwaitingResult>(state))
            {
                state = ResultValue{std::move(value)};
            }
        });
    }

private:
    Synchronized<State> state;
};

} // namespace Teide::detail

namespace Teide
{

template <class T>
class ListenSender;

template <class T>
class ListenReceiver;

template <class T>
auto Listen(ListenReceiver<T>& receiver) -> ListenSender<T>;

template <class T>
class ListenSender
{
public:
    using sender_concept = ex::sender_t;

    using completion_signatures = detail::CompletionSignaturesFor<T>;

    template <class Receiver>
    class Operation : Immovable
    {
    public:
        Operation(std::shared_ptr<detail::CompletionState<T>> completion, Receiver&& receiver) :
            m_completion{std::move(completion)}, m_receiver{std::move(receiver)}
        {}

        void start() noexcept
        {
            if (m_completion->SetReceiver(m_receiver))
            {
                m_completion.reset();
            }
        }

    private:
        std::shared_ptr<detail::CompletionState<T>> m_completion;
        Receiver m_receiver;
    };

    template <ex::receiver R>
    friend auto tag_invoke(ex::connect_t /*tag*/, const ListenSender& sender, R&& receiver) -> Operation<R>
    {
        return Operation(std::move(sender.m_completion), std::forward<R>(receiver));
    }

private:
    explicit ListenSender(std::shared_ptr<detail::CompletionState<T>> completion) : m_completion{std::move(completion)}
    {}

    std::shared_ptr<detail::CompletionState<T>> m_completion;

    friend auto Listen<T>(ListenReceiver<T>& receiver) -> ListenSender<T>;
};

template <class T>
class ListenReceiver : MoveOnly
{
public:
    using receiver_concept = ex::receiver_t;

    void set_value(T&& value) &&
    {
        if (auto comp = m_completion.lock())
        {
            comp->SetValue(std::move(value));
        }
    }

    void set_stopped() &&
    {
        if (auto comp = m_completion.lock())
        {
            comp->SetStopped();
        }
    }

    bool expired() const { return m_completion.expired(); }

private:
    std::weak_ptr<detail::CompletionState<T>> m_completion;

    friend auto Listen<T>(ListenReceiver<T>& receiver) -> ListenSender<T>;
};

template <class T>
auto Listen(ListenReceiver<T>& receiver) -> ListenSender<T>
{
    if (auto comp = receiver.m_completion.lock())
    {
        return ListenSender<T>(comp);
    }

    auto comp = std::make_shared<detail::CompletionState<T>>();
    receiver.m_completion = comp;

    return ListenSender<T>(comp);
}

} // namespace Teide
