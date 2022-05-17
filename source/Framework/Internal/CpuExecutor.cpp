
#include "Framework/Internal/CpuExecutor.h"

using namespace std::chrono_literals;

CpuExecutor::CpuExecutor(std::uint32_t numThreads) : m_executor(numThreads)
{
	m_schedulerThread = std::jthread([this, stop_token = m_schedulerStopSource.get_token()] {
		constexpr auto timeout = 2ms;

		while (!stop_token.stop_requested())
		{
			std::this_thread::sleep_for(timeout);

			auto lock = std::scoped_lock(m_schedulerMutex);

			// If there are futures, check if any have a value
			for (auto& [future, callback, promise] : m_scheduledTasks)
			{
				if (future.wait_for(0s) == std::future_status::ready)
				{
					// Found one; execute the scheduled task
					LaunchTaskImpl([callback, promise] {
						callback();
						promise->set_value();
					});

					// Mark it as done
					callback = nullptr;
				}
			}

			// Remove done tasks
			std::erase_if(m_scheduledTasks, [](const auto& entry) { return entry.callback == nullptr; });
		}
	});
}

CpuExecutor::~CpuExecutor()
{
	m_schedulerStopSource.request_stop();
	WaitForTasks();
}

void CpuExecutor::WaitForTasks()
{
	m_executor.wait_for_all();

	const auto hasScheduledTasks = [this] {
		auto lock = std::scoped_lock(m_schedulerMutex);
		return !m_scheduledTasks.empty();
	};

	while (hasScheduledTasks())
	{
		// busy loop for now
		std::this_thread::sleep_for(2ms);
		m_executor.wait_for_all();
	}

	m_executor.wait_for_all();
}
