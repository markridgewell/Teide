
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Task.h"

#include <function2/function2.hpp>
#include <taskflow/taskflow.hpp>

#include <future>
#include <memory>
#include <mutex>
#include <thread>

namespace Teide
{

namespace detail
{
    template <class T>
    struct PromiseHelper
    {
        using type = std::promise<std::optional<T>>;
    };

    template <>
    struct PromiseHelper<void>
    {
        using type = std::promise<void>;
    };

    template <class Ret, class Arg>
    struct UnaryFunctionHelper
    {
        using type = fu2::unique_function<Ret(Arg)>;
    };

    template <class Ret>
    struct UnaryFunctionHelper<Ret, void>
    {
        using type = fu2::unique_function<Ret()>;
    };

} // namespace detail

template <class T = void>
using Promise = detail::PromiseHelper<T>::type;

template <class F, class... Args>
    requires std::invocable<F, Args...>
using TaskForCallable = Task<std::invoke_result_t<F, Args...>>;


class CpuExecutor
{
public:
    explicit CpuExecutor(uint32 numThreads);
    ~CpuExecutor();

    CpuExecutor(const CpuExecutor&) = delete;
    CpuExecutor(CpuExecutor&&) = delete;
    CpuExecutor& operator=(const CpuExecutor&) = delete;
    CpuExecutor& operator=(CpuExecutor&&) = delete;

    template <std::invocable<> F>
    auto LaunchTask(F&& f) -> TaskForCallable<F>
    {
        return LaunchTaskImpl(std::forward<F>(f));
    }

    template <std::invocable<uint32_t> F>
    auto LaunchTask(F&& f) -> TaskForCallable<F, uint32_t>
    {
        return LaunchTaskImpl([this, f = std::forward<F>(f)]() mutable {
            const uint32_t taskIndex = m_executor.this_worker_id();
            return std::forward<F>(f)(taskIndex);
        });
    }

    template <std::invocable<> F>
    auto LaunchTask(F&& f, Task<> dependency) -> TaskForCallable<F>
    {
        const auto lock = std::scoped_lock(m_schedulerMutex);

        auto newTask = MakeScheduledTask<void>(std::move(dependency), std::forward<F>(f));
        auto future = newTask->GetFuture();
        m_scheduledTasks.emplace_back(std::move(newTask));
        return future;
    }

    template <class T, std::invocable<T> F>
    auto LaunchTask(F&& f, std::shared_future<std::optional<T>> dependency) -> TaskForCallable<F, T>
    {
        const auto lock = std::scoped_lock(m_schedulerMutex);

        auto newTask = MakeScheduledTask<T>(std::move(dependency), std::forward<F>(f));
        auto future = newTask->GetFuture();
        m_scheduledTasks.emplace_back(std::move(newTask));
        return future;
    }

    uint32 GetThreadIndex() const { return m_executor.this_worker_id(); }

    void WaitForTasks();

private:
    template <class Ret, class Arg>
    using UnaryFunction = detail::UnaryFunctionHelper<Ret, Arg>::type;

    template <class F, class... Args>
        requires std::invocable<F, Args...>
    auto LaunchTaskImpl(F&& f, Args&&... args)
    {
        return m_executor.async(std::forward<F>(f), std::forward<Args>(args)...);
    }

    tf::Executor m_executor;

    struct AbstractScheduledTask
    {
        virtual ~AbstractScheduledTask() = default;
        virtual bool IsReady() const = 0;
        virtual void Execute(CpuExecutor& executor) = 0;
    };

    template <class InT, class OutT>
    struct ConcreteScheduledTask :
        AbstractScheduledTask,
        public std::enable_shared_from_this<ConcreteScheduledTask<InT, OutT>>
    {
        Task<InT> future;
        UnaryFunction<OutT, InT> callback;
        Promise<OutT> promise;

        OutT InvokeCallback() requires std::is_void_v<InT> { return callback(); }

        OutT InvokeCallback() { return callback(future.get().value()); }

        void ExecuteTask() requires std::is_void_v<OutT>
        {
            InvokeCallback();
            promise.set_value();
        }

        void ExecuteTask()
        {
            auto value = InvokeCallback();
            promise.set_value(std::move(value));
        }

        ConcreteScheduledTask(Task<InT> future, UnaryFunction<OutT, InT> callback) :
            future{std::move(future)}, callback{std::move(callback)}
        {}

        bool IsReady() const override { return future.wait_for(std::chrono::seconds{0}) == std::future_status::ready; }

        void Execute(CpuExecutor& executor) override
        {
            // Make sure the future actually has a result (get will throw exception if not)
            static_cast<void>(future.get());

            // Found one; execute the scheduled task
            executor.LaunchTaskImpl([t = this->shared_from_this()]() { t->ExecuteTask(); });
        }

        Task<OutT> GetFuture() { return promise.get_future(); }
    };

    template <class T, class U>
    auto MakeScheduledTask(Task<T> future, UnaryFunction<U, T> callback)
    {
        return std::make_shared<ConcreteScheduledTask<T, U>>(std::move(future), std::move(callback));
    }

    template <class T, class F>
        requires(std::is_void_v<T>)
    auto MakeScheduledTask(Task<T> future, F&& callback)
    {
        using U = std::invoke_result_t<F>;
        return MakeScheduledTask<T, U>(std::move(future), std::forward<F>(callback));
    }

    template <class T, class F>
        requires(!std::is_void_v<T>)
    auto MakeScheduledTask(Task<T> future, F&& callback)
    {
        using U = std::invoke_result_t<F, T>;
        return MakeScheduledTask<T, U>(std::move(future), std::forward<F>(callback));
    }

    // Ideally we'd store unique_ptrs here. However, the Taskflow library doesn't support move-only
    // function types, which causes problems as std::future and std::promise are both move-only.
    // This means the lambda that invokes a scheduled task will also be move-only, *unless* we store
    // the task object in a shared_ptr and capture it by copy.
    std::vector<std::shared_ptr<AbstractScheduledTask>> m_scheduledTasks;

    std::mutex m_schedulerMutex;
    std::stop_source m_schedulerStopSource;
    std::jthread m_schedulerThread;
};

} // namespace Teide
