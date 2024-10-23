
#include "Teide/Assert.h"
#include "Teide/TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <exception>

#if __linux__
#    include <sys/ioctl.h>
#endif

namespace
{
std::optional<std::size_t> GetNumConsoleColumns()
{
#if __linux__
    if (isatty(1))
    {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return std::size_t{w.ws_col};
    }
    if (const char* cols = getenv("COLUMNS"))
    {
        return std::atol(cols);
    }
#endif
    return std::nullopt;
}

bool AssertDie(std::string_view msg, std::string_view expression, Teide::SourceLocation location)
{
    std::cout << location.file_name();
    if (location.line() > 0)
    {
        std::cout << '(' << location.line() << ')';
    }
    std::cout << ": ";
    if (std::strlen(location.function_name()) > 0)
    {
        std::cout << location.function_name() << ": ";
    }
    if (expression.empty())
    {
        std::cout << msg << '\n';
    }
    else
    {
        std::cout << "Assertion failed: " << expression << ": " << msg << '\n';
    }
    std::terminate();
}

} // namespace

#if defined(_WIN32) && __has_include("StackWalker.h")
#    define STACKWALKER_ENABLED
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
    void OnTestProgramStart(const testing::UnitTest& unitTest) override { m_testCount = unitTest.test_to_run_count(); }

    void OnTestStart(const testing::TestInfo& testInfo) override
    {
        ++m_currentTestIndex;
        m_testName = fmt::format("{}.{}", testInfo.test_suite_name(), testInfo.name());
        const auto percentComplete = m_currentTestIndex * 100 / m_testCount;
        std::string line = fmt::format("[{:>3}%] {}", percentComplete, m_testName);
        if (const auto numCols = GetNumConsoleColumns())
        {
            if (line.size() < *numCols)
            {
                line.append(*numCols - line.size(), ' ');
            }
            else if (line.size() > *numCols)
            {
                line.resize(*numCols - 4);
                line.append("...");
            }
            std::cout << '\r' << line << std::flush;
        }
        else
        {
            std::cout << line << '\n';
        }
        m_failure = false;
        m_output = {};
        m_logger = spdlog::default_logger();
        spdlog::set_default_logger(
            std::make_shared<spdlog::logger>("", std::make_shared<spdlog::sinks::ostream_sink_mt>(m_output)));
    }

    void OnTestEnd(const testing::TestInfo& testInfo) override
    {
        if (testInfo.result()->Failed())
        {
            fmt::println("");
        }
    }

    void OnTestPartResult(const testing::TestPartResult& result) override
    {
        if (result.failed() && !m_failure)
        {
            if (GetNumConsoleColumns())
            {
                std::cout << '\r';
            }
            std::cout << "\033[31m[FAIL]\033[39m " << m_testName << "  " << std::endl;
            m_failure = true;

            spdlog::set_default_logger(m_logger);
            const auto logString = m_output.str();
            std::cout << logString;
        }

        if (result.failed())
        {
            fmt::print(
                "{}({}): \033[31massertion failed\033[39m: {}", result.file_name(), result.line_number(), result.message());
        }
    }

    void OnTestProgramEnd(const testing::UnitTest& unitTest) override
    {
        if (const auto numCols = GetNumConsoleColumns())
        {
            fmt::print("\r{:{}}\r", ' ', *numCols);
        }

        fmt::print("Test results: ");
        if (unitTest.successful_test_count() > 0)
        {
            fmt::print("\033[32m");
        }
        fmt::print("{} passed\033[39m, ", unitTest.successful_test_count());

        if (unitTest.failed_test_count() > 0)
        {
            fmt::print("\033[31m");
        }
        fmt::print("{} failed\033[39m, ", unitTest.failed_test_count());

        if (unitTest.skipped_test_count() > 0)
        {
            fmt::print("\033[33m");
        }
        fmt::print("{} skipped\033[39m", unitTest.skipped_test_count());

        std::cout << '\n';
    }

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::stringstream m_output;
    bool m_failure = false;
    std::string m_testName;
    int m_currentTestIndex = 0;
    int m_testCount = 0;
};

int main(int argc, char** argv)
{
#ifdef STACKWALKER_ENABLED
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
        delete listeners.Release(listeners.default_result_printer());
        // gtest demands an owning raw pointer to be passed in here
        listeners.Append(new LogSuppressor); // NOLINT(cppcoreguidelines-owning-memory)
    }

    if (Teide::IsDebuggerAttached())
    {
        spdlog::info("Debugger detected");
    }
    else
    {
        Teide::SetAssertHandler(&AssertDie);
    }

    testing::FLAGS_gtest_break_on_failure = Teide::IsDebuggerAttached();

    return RUN_ALL_TESTS();
}
