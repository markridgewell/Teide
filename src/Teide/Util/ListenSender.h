
#pragma once

#include "Teide/Assert.h"
#include "Teide/Util/ThreadUtils.h"
#include "Teide/Util/TypeHelpers.h"

#include <stdexec/execution.hpp>

#include <functional>
#include <type_traits>
#include <utility>
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

    template <ex::receiver_of<Sigs> Receiver>
    auto SetReceiver(Receiver& receiver) -> bool
    {
        TEIDE_ASSERT(!std::holds_alternative<OnResult>(state));

        if (std::holds_alternative<Canceled>(state))
        {
            std::move(receiver).set_stopped();
            return true;
        }

        if (auto* result = std::get_if<ResultValue>(&state))
        {
            std::move(receiver).set_value(std::move(*result));
            return true;
        }

        if (std::holds_alternative<AwaitingResult>(state))
        {
            state = OnResult{
                .setValue = [&receiver](T value) { std::move(receiver).set_value(std::move(value)); },
                .setStopped = [&receiver] { std::move(receiver).set_stopped(); },
            };
        }

        return false;
    }

    void SetValue(T value)
    {
        TEIDE_ASSERT(!std::holds_alternative<ResultValue>(state) || std::holds_alternative<Canceled>(state));

        if (auto* onResult = std::get_if<OnResult>(&state))
        {
            onResult->setValue(std::move(value));
        }

        if (std::holds_alternative<AwaitingResult>(state))
        {
            state = ResultValue{std::move(value)};
        }
    }

    void SetStopped()
    {
        TEIDE_ASSERT(!std::holds_alternative<ResultValue>(state) || std::holds_alternative<Canceled>(state));

        if (auto* onResult = std::get_if<OnResult>(&state))
        {
            onResult->setStopped();
        }

        if (std::holds_alternative<AwaitingResult>(state))
        {
            state = Canceled{};
        }
    }

private:
    State state;
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

    using Completion = detail::CompletionState<T>;
    using CompletionSharedPtr = std::shared_ptr<Synchronized<Completion>>;

    template <class Receiver>
    class Operation : Immovable
    {
    public:
        Operation(CompletionSharedPtr completion, Receiver&& receiver) :
            m_completion{std::move(completion)}, m_receiver{std::move(receiver)}
        {}

        Operation(CompletionSharedPtr completion, Receiver& receiver)
            requires std::is_lvalue_reference_v<Receiver>
            : m_completion{std::move(completion)}, m_receiver{receiver}
        {}

        void start() noexcept
        {
            if (m_completion->Lock(&Completion::template SetReceiver<Receiver>, m_receiver))
            {
                m_completion.reset();
            }
        }

    private:
        CompletionSharedPtr m_completion;
        Receiver m_receiver;
    };

    template <class Self, ex::receiver R>
    auto connect(this Self&& self, R&& receiver)
    {
        return Operation<R>(std::forward<Self>(self).m_completion, std::forward<R>(receiver));
    }

private:
    explicit ListenSender(CompletionSharedPtr completion) : m_completion{std::move(completion)} {}

    CompletionSharedPtr m_completion;

    friend auto Listen<T>(ListenReceiver<T>& receiver) -> ListenSender<T>;
};

template <class T>
class ListenReceiver : MoveOnly
{
public:
    using receiver_concept = ex::receiver_t;

    using Completion = detail::CompletionState<T>;
    using CompletionWeakPtr = std::weak_ptr<Synchronized<Completion>>;

    void set_value(T&& value) &&
    {
        TEIDE_ASSERT(not m_called);
        m_called = true;
        if (auto comp = m_completion.lock())
        {
            comp->Lock(&Completion::SetValue, std::move(value));
        }
    }

    void set_stopped() &&
    {
        TEIDE_ASSERT(not m_called);
        m_called = true;
        if (auto comp = m_completion.lock())
        {
            comp->Lock(&Completion::SetStopped);
        }
    }

    bool expired() const { return m_completion.expired(); }

private:
    CompletionWeakPtr m_completion;
    bool m_called = false;

    friend auto Listen<T>(ListenReceiver<T>& receiver) -> ListenSender<T>;
};

template <class T>
auto Listen(ListenReceiver<T>& receiver) -> ListenSender<T>
{
    if (auto comp = receiver.m_completion.lock())
    {
        return ListenSender<T>(comp);
    }

    auto comp = std::make_shared<Synchronized<detail::CompletionState<T>>>();
    receiver.m_completion = comp;

    return ListenSender<T>(comp);
}

} // namespace Teide
