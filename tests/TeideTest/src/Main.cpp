
#include "Teide/TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

class LogSuppressor : public testing::EmptyTestEventListener
{
public:
    void OnTestStart(const testing::TestInfo&) override
    {
        m_output.clear();
        m_logger = spdlog::default_logger();
        spdlog::set_default_logger(
            std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::ostream_sink_mt>(m_output)));
    }

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

    void OnTestEnd(const testing::TestInfo&) override { spdlog::set_default_logger(m_logger); }

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::stringstream m_output;
};

int main(int argc, char** argv)
{
    for (std::string_view arg : std::span(argv, argc).subspan<1>())
    {
        if (arg == "-s" || arg == "--sw-render")
        {
            Teide::EnableSoftwareRendering();
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            spdlog::set_level(spdlog::level::debug);
            spdlog::debug("Verbose logging enabled");
        }
    }

    testing::InitGoogleTest(&argc, argv);

    if (spdlog::get_level() > spdlog::level::debug)
    {
        testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
        // gtest demands an owning raw pointer to be passed in here
        listeners.Append(new LogSuppressor); // NOLINT(cppcoreguidelines-owning-memory)
    }

    return RUN_ALL_TESTS();
}
