
#pragma once

#include <taskflow/taskflow.hpp>

#include <functional>
#include <future>
#include <mutex>
#include <thread>

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
	auto LaunchTask(F&& f) -> std::shared_future<void>
	{
		return LaunchTaskImpl(std::forward<F>(f)).share();
	}

	template <std::invocable<uint32_t> F>
	auto LaunchTask(F&& f) -> std::shared_future<void>
	{
		return LaunchTask([this, f = std::forward<F>(f)]() mutable {
			const uint32_t taskIndex = m_executor.this_worker_id();
			std::forward<F>(f)(taskIndex);
		});
	}

	template <std::invocable<> F>
	auto LaunchTask(F&& f, std::shared_future<void> dependency) -> std::shared_future<void>
	{
		const auto lock = std::scoped_lock(m_schedulerMutex);

		auto& task = m_scheduledTasks.emplace_back(
		    std::move(dependency), std::forward<F>(f), std::make_shared<std::promise<void>>());
		return task.promise->get_future().share();
	}

	void WaitForTasks();

private:
	template <class F, class... Args>
		requires std::invocable<F, Args...>
	auto LaunchTaskImpl(F&& f, Args&&... args) -> std::future<void>
	{
		return m_executor.async(std::forward<F>(f), std::forward<Args>(args)...);
	}

	tf::Executor m_executor;

	struct ScheduledTask
	{
		std::shared_future<void> future;
		std::function<void()> callback;
		std::shared_ptr<std::promise<void>> promise;
	};
	std::vector<ScheduledTask> m_scheduledTasks;

	std::mutex m_schedulerMutex;
	std::stop_source m_schedulerStopSource;
	std::jthread m_schedulerThread;
};
