
#include "Framework/Internal/CpuExecutor.h"

#include "Framework/Buffer.h"
#include "Framework/Internal/Vulkan.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <memory>

using namespace testing;
using namespace std::chrono_literals;
using std::this_thread::sleep_for;

namespace
{

TEST(CpuExecutorTest, LaunchOneTask)
{
	auto executor = CpuExecutor(2);

	int result = 0;
	executor.LaunchTask([&] {
		sleep_for(100ms);
		result = 42;
	});
	executor.WaitForTasks();

	EXPECT_THAT(result, Eq(42));
}

TEST(CpuExecutorTest, LaunchOneTaskWithWorkerID)
{
	auto executor = CpuExecutor(2);

	std::uint32_t result[4] = {9999, 9999, 9999, 9999};
	for (std::uint32_t i = 0; i < 4; i++)
	{
		executor.LaunchTask([&result, i](std::uint32_t workerID) {
			sleep_for(100ms);
			result[i] = workerID;
		});
	}
	executor.WaitForTasks();

	EXPECT_THAT(result, ElementsAre(Le(2u), Le(2u), Le(2u), Le(2u)));
}

TEST(CpuExecutorTest, LaunchIndependentTasks)
{
	auto executor = CpuExecutor(2);

	int result[4] = {0, 0, 0, 0};
	executor.LaunchTask([&] {
		sleep_for(100ms);
		result[0] = 42;
	});
	executor.LaunchTask([&] {
		sleep_for(100ms);
		result[1] = 21;
	});
	executor.LaunchTask([&] {
		sleep_for(100ms);
		result[2] = 84;
	});
	executor.LaunchTask([&] {
		sleep_for(100ms);
		result[3] = 24;
	});
	executor.WaitForTasks();

	EXPECT_THAT(result, ElementsAre(42, 21, 84, 24));
}

TEST(CpuExecutorTest, LaunchDependentTasksAndWaitForTasks)
{
	auto executor = CpuExecutor(2);

	int result[4] = {0, 0, 0, 0};
	const auto task0 = executor.LaunchTask([&] {
		sleep_for(100ms);
		result[0] = 42;
	});
	const auto task1 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[1] = result[0] / 2;
	    },
	    task0);
	const auto task2 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[2] = result[1] * 4;
	    },
	    task1);
	const auto task3 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[3] = result[2] - 60;
	    },
	    task2);

	(void)task3;
	executor.WaitForTasks();

	EXPECT_THAT(result, ElementsAre(42, 21, 84, 24));
}

TEST(CpuExecutorTest, LaunchDependentTasksAndWaitIndividually)
{
	auto executor = CpuExecutor(2);

	int result[4] = {0, 0, 0, 0};
	const auto task0 = executor.LaunchTask([&] {
		sleep_for(100ms);
		result[0] = 42;
	});
	const auto task1 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[1] = result[0] / 2;
	    },
	    task0);
	const auto task2 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[2] = result[0] * 2;
	    },
	    task0);
	const auto task3 = executor.LaunchTask(
	    [&] {
		    sleep_for(100ms);
		    result[3] = result[2] - 60;
	    },
	    task2);

	task0.get();
	task1.get();
	task2.get();
	task3.get();

	EXPECT_THAT(result, ElementsAre(42, 21, 84, 24));
}
} // namespace
