
#pragma once

#include <taskflow/taskflow.hpp>

#include <functional>
#include <future>
#include <mutex>
#include <thread>

namespace detail
{
template <class T>
struct TaskHelper
{
	using type = std::shared_future<std::optional<T>>;
};

template <>
struct TaskHelper<void>
{
	using type = std::shared_future<void>;
};

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
} // namespace detail

template <class T = void>
using Task = detail::TaskHelper<T>::type;

template <class T = void>
using Promise = detail::PromiseHelper<T>::type;

template <class F, class... Args>
	requires std::invocable<F, Args...>
using TaskForCallable = Task<std::invoke_result_t<F, Args...>>;


class CpuExecutor
{
public:
	explicit CpuExecutor(std::uint32_t numThreads);
	~CpuExecutor();

	CpuExecutor(const CpuExecutor&) = delete;
	CpuExecutor(CpuExecutor&&) = delete;
	CpuExecutor& operator=(const CpuExecutor&) = delete;
	CpuExecutor& operator=(CpuExecutor&&) = delete;

	template <std::invocable<> F>
	auto LaunchTask(F&& f) -> TaskForCallable<F>
	{
		return LaunchTaskImpl(std::forward<F>(f)).share();
	}

	template <std::invocable<uint32_t> F>
	auto LaunchTask(F&& f) -> TaskForCallable<F, uint32_t>
	{
		return LaunchTask([this, f = std::forward<F>(f)]() mutable {
			const uint32_t taskIndex = m_executor.this_worker_id();
			std::forward<F>(f)(taskIndex);
		});
	}

	template <std::invocable<> F>
	auto LaunchTask(F&& f, Task<> dependency) -> TaskForCallable<F>
	{
		const auto lock = std::scoped_lock(m_schedulerMutex);

		auto newTask = MakeScheduledTask<void>(std::move(dependency), std::forward<F>(f));
		auto future = newTask->promise->get_future().share();
		m_scheduledTasks.emplace_back(std::move(newTask));
		return future;
	}

	template <class T, std::invocable<T> F>
	auto LaunchTask(F&& f, std::shared_future<std::optional<T>> dependency) -> TaskForCallable<F, T>
	{
		const auto lock = std::scoped_lock(m_schedulerMutex);

		auto newTask = MakeScheduledTask<T>(std::move(dependency), std::forward<F>(f));
		auto future = newTask->promise->get_future().share();
		m_scheduledTasks.emplace_back(std::move(newTask));
		return future;
	}

	void WaitForTasks();

private:
	template <class F, class... Args>
		requires std::invocable<F, Args...>
	auto LaunchTaskImpl(F&& f, Args&&... args)
	{
		return m_executor.async(std::forward<F>(f), std::forward<Args>(args)...);
	}

	tf::Executor m_executor;

	template <class Ret, class Arg = void>
	using UnaryFunction = std::function<Ret(Arg)>;

	struct AbstractScheduledTask
	{
		bool done = false;

		virtual ~AbstractScheduledTask() = default;
		virtual void ExecuteIfReady(CpuExecutor& executor) = 0;
	};

	template <class T, class U>
	struct ConcreteScheduledTask : AbstractScheduledTask
	{
		Task<T> future;
		std::function<U(T)> callback;
		std::shared_ptr<Promise<U>> promise;

		ConcreteScheduledTask(Task<T> future, std::function<U(T)> callback, std::shared_ptr<Promise<U>> promise) :
		    future{std::move(future)}, callback{std::move(callback)}, promise{std::move(promise)}
		{}

		void ExecuteIfReady(CpuExecutor& executor) override
		{
			using namespace std::chrono_literals;

			if (future.wait_for(0s) == std::future_status::ready)
			{
				// Found one; execute the scheduled task
				executor.LaunchTaskImpl([future = future, callback = callback, promise = promise] {
					if constexpr (std::is_void_v<T> && std::is_void_v<U>)
					{
						callback();
						promise->set_value();
					}
					else if constexpr (std::is_void_v<T> && !std::is_void_v<U>)
					{
						auto value = callback();
						promise->set_value(std::move(value));
					}
					else if constexpr (!std::is_void_v<T> && std::is_void_v<U>)
					{
						callback(future.get().value());
						promise->set_value();
					}
					else if constexpr (!std::is_void_v<T> && !std::is_void_v<U>)
					{
						auto value = callback(future.get().value());
						promise->set_value(std::move(value));
					}
				});

				// Mark it as done
				done = true;
			}
		}
	};

	template <class T, class U>
	auto MakeScheduledTask(Task<T> future, UnaryFunction<U, T> callback)
	{
		return std::make_unique<ConcreteScheduledTask<T, U>>(
		    std::move(future), std::move(callback), std::make_shared<Promise<U>>());
	}

	template <class T, class F>
	auto MakeScheduledTask(Task<T> future, F callback)
	{
		if constexpr (!std::is_void_v<T>)
		{
			using U = std::invoke_result_t<F, T>;
			return MakeScheduledTask<T, U>(std::move(future), std::move(callback));
		}
		else if constexpr (std::is_void_v<T>)
		{
			using U = std::invoke_result_t<F>;
			return MakeScheduledTask<T, U>(std::move(future), std::move(callback));
		}
	}

	std::vector<std::unique_ptr<AbstractScheduledTask>> m_scheduledTasks;

	std::mutex m_schedulerMutex;
	std::stop_source m_schedulerStopSource;
	std::jthread m_schedulerThread;
};
