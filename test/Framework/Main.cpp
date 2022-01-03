
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

class LogSuppressor : public testing::EmptyTestEventListener
{
public:
	LogSuppressor()
	{
		spdlog::set_default_logger(
		    std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::ostream_sink_mt>(m_output)));
	}

	void OnTestStart(const testing::TestInfo&) override { m_output.clear(); }

	// Called after a failed assertion or a SUCCEED() invocation.
	void OnTestPartResult(const testing::TestPartResult& result) override
	{
		if (result.failed())
		{
			std::cerr << "\n";
			std::cerr << "Log output for this test:\n";
			std::cerr << "=========================\n";
			std::cerr << m_output.str();
			std::cerr << "=========================\n";
		}
	}

private:
	std::stringstream m_output;
};

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	// testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
	// listeners.Append(new LogSuppressor);

	return RUN_ALL_TESTS();
}
