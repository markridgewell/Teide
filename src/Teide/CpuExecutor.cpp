
#include "CpuExecutor.h"

#include "Teide/Assert.h"
#include "Teide/Util/ThreadUtils.h"

#include <spdlog/spdlog.h>

namespace Teide
{

namespace
{
    thread_local bool s_threadHasName = false;
}

class WorkerInterface : public ::tf::WorkerInterface
{
    void scheduler_prologue(tf::Worker& worker [[maybe_unused]]) override
    {
        if (!s_threadHasName)
        {
            SetCurrentTheadName(fmt::format("Worker{}", worker.id()));
            s_threadHasName = true;
        }
    }
    void scheduler_epilogue(tf::Worker& worker [[maybe_unused]], std::exception_ptr ptr [[maybe_unused]]) override
    {
        if (ptr)
        {
            try
            {
                std::rethrow_exception(ptr);
            }
            catch (const std::exception& e)
            {
                TEIDE_BREAK("Unhandled exception in worker thread: {}", e.what());
            }
        }
    }
};

CpuExecutor::CpuExecutor(uint32 numThreads) : m_executor(numThreads, std::make_shared<WorkerInterface>())
{
    m_schedulerThread = std::thread([this] {
        SetCurrentTheadName("CpuExecutor");
        constexpr auto timeout = std::chrono::milliseconds{2};

        while (!m_schedulerStop)
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
    m_schedulerStop = true;
    m_schedulerThread.join();
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
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
        m_executor.wait_for_all();
    }

    m_executor.wait_for_all();
}

} // namespace Teide
