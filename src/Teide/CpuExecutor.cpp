
#include "CpuExecutor.h"

using namespace std::chrono_literals;

namespace Teide
{
class WorkerInterface : public ::tf::WorkerInterface
{
    void scheduler_prologue(tf::Worker& worker [[maybe_unused]]) override {}
    void scheduler_epilogue(tf::Worker& worker [[maybe_unused]], std::exception_ptr ptr [[maybe_unused]]) override
    {
        assert(ptr == nullptr);
    }
};

CpuExecutor::CpuExecutor(uint32 numThreads) : m_executor(numThreads, std::make_shared<WorkerInterface>())
{
    m_schedulerThread = std::jthread([this, stop_token = m_schedulerStopSource.get_token()] {
        constexpr auto timeout = 2ms;

        while (!stop_token.stop_requested())
        {
            std::this_thread::sleep_for(timeout);

            auto lock = std::scoped_lock(m_schedulerMutex);

            // If there are futures, check if any have a value
            for (auto& task : m_scheduledTasks)
            {
                if (task->IsReady())
                {
                    auto t = std::exchange(task, nullptr);
                    t->Execute(*this);
                }
            }

            // Remove done tasks
            std::erase(m_scheduledTasks, nullptr);
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

} // namespace Teide
