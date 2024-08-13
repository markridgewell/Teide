
#include "CpuExecutor.h"

#include "Teide/Assert.h"
#include "Teide/Util/ThreadUtils.h"

#include <spdlog/spdlog.h>

namespace Teide
{

CpuExecutor::CpuExecutor(uint32 numThreads) : m_executor(numThreads)
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
