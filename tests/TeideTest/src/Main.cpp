
#include "Teide/TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#    include "StackWalker.h"

class MyStackWalker : public StackWalker
{
    virtual void OnOutput(LPCSTR szText) { std::puts(szText); }
};

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptions [[maybe_unused]])
{
    MyStackWalker sw;
    sw.ShowCallstack(GetCurrentThread(), exceptions->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif

class LogSuppressor : public testing::EmptyTestEventListener
{
public:
    void OnTestStart(const testing::TestInfo& /*unused*/) override
    {
        m_output = {};
        m_logger = spdlog::default_logger();
        spdlog::set_default_logger(
            std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::ostream_sink_mt>(m_output)));
    }

    void OnTestEnd(const testing::TestInfo& testInfo) override
    {
        spdlog::set_default_logger(m_logger);
        const auto logString = m_output.str();
        if (testInfo.result()->Failed() && !logString.empty())
        {
            std::cerr << "\n";
            std::cerr << "Log output for this test:\n";
            std::cerr << "=========================\n";
            std::cerr << logString;
            std::cerr << "=========================\n";
        }
    }

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::stringstream m_output;
};

int main(int argc, char** argv)
{
#ifdef _WIN32
    spdlog::info("Setting exception handler...");
    SetUnhandledExceptionFilter(ExceptionHandler);
#endif
    spdlog::flush_on(spdlog::level::err);

    for (const std::string_view arg : std::span(argv, static_cast<std::size_t>(argc)).subspan<1>())
    {
        if (arg == "-s" || arg == "--sw-render")
        {
            Teide::EnableSoftwareRendering();
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            spdlog::set_level(spdlog::level::debug);
            spdlog::info("Verbose logging enabled");
        }
        else if (arg == "--windowless")
        {
            g_windowless = true;
            spdlog::info("Windowless environment indicated: skipping surface tests");
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
