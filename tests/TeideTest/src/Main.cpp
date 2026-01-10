
#include "Teide/Assert.h"
#include "Teide/TestUtils.h"

#include <cpptrace/basic.hpp>
#include <cpptrace/cpptrace.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <ranges>

#if __linux__
#    include <sys/ioctl.h>
#endif

#if defined(_WIN32) && __has_include("StackWalker.h")
#    define STACKWALKER_ENABLED
#    include "StackWalker.h"
#endif

namespace
{
std::optional<std::size_t> GetNumConsoleColumns()
{
#if __linux__
    if (isatty(STDOUT_FILENO))
    {
        winsize w{};
        const int err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // NOLINT(cppcoreguidelines-pro-type-vararg)
        if (err != 0 || w.ws_col == 0)
        {
            return std::nullopt;
        }
        return std::size_t{w.ws_col};
    }
    if (const char* cols = getenv("COLUMNS")) // NOLINT(concurrency-mt-unsafe)
    {
        return std::atol(cols);
    }
#endif
    return std::nullopt;
}

bool AssertThrow(std::string_view msg, std::string_view expression, Teide::SourceLocation location)
{
    struct AssertException
    {};

    const auto trace = cpptrace::generate_trace();
    for (const auto& frame : trace | std::views::drop(1))
    {
        if (frame.filename.empty()
            || frame.filename.ends_with(".so")
            || frame.filename.contains("/exports/installed")
            || frame.filename.contains("/vcpkg/buildtrees/"))
        {
            continue;
        }
        if (frame.symbol.contains("::DebugCallback("))
        {
            continue;
        }
        location = Teide::SourceLocation(frame.line.value_or(0), 0, frame.filename.c_str(), frame.symbol.c_str());
        break;
    }

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

    const auto* curTest = testing::UnitTest::GetInstance()->current_test_info();
    if (curTest)
    {
        std::cout << "Current test: " << curTest->name() << "\n";
        GTEST_NONFATAL_FAILURE_("Assert failed while executing test");
    }
    trace.print(std::cout, true);
    throw AssertException{};
}

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
                fmt::print("\r");
            }
            fmt::println("\033[31m[FAIL]\033[39m {}", m_testName);
            m_failure = true;

            spdlog::set_default_logger(m_logger);
            const auto logString = m_output.str();
            std::cout << logString;
        }

        if (result.failed())
        {
            if (result.file_name())
            {
                fmt::print(
                    "{}({}): \033[31massertion failed\033[39m: {}", result.file_name(), result.line_number(),
                    result.message());
            }
            else
            {
                fmt::print("\033[31mtest error\033[39m: {}", result.message());
            }
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

int Run(int argc, char** argv)
{
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
        delete listeners.Release(listeners.default_result_printer()); // NOLINT(cppcoreguidelines-owning-memory)
        // gtest demands an owning raw pointer to be passed in here
        listeners.Append(new LogSuppressor); // NOLINT(cppcoreguidelines-owning-memory)
    }

    if (Teide::IsDebuggerAttached())
    {
        spdlog::info("Debugger detected");
    }
    else
    {
        Teide::SetAssertHandler(&AssertThrow);
    }

    testing::FLAGS_gtest_break_on_failure = Teide::IsDebuggerAttached();

    return RUN_ALL_TESTS();
}

} // namespace

#ifdef STACKWALKER_ENABLED
class MyStackWalker : public StackWalker
{
public:
    MyStackWalker() :
        StackWalker(
            StackWalker::RetrieveSymbol | StackWalker::RetrieveLine | StackWalker::SymAll, nullptr,
            GetCurrentProcessId(), GetCurrentProcess())
    {}

    virtual void OnOutput(LPCSTR szText)
    {
        std::string_view text = szText;
        if (text.ends_with("\n"))
        {
            text.remove_suffix(1);
        }
        if (text.ends_with("\r"))
        {
            text.remove_suffix(1);
        }
        spdlog::info("{}", text);
    }
};
MyStackWalker stackWalker;

LONG WINAPI ExpFilter(EXCEPTION_POINTERS* pExp, DWORD /*dwExpCode*/)
{
    spdlog::error("Caught SEH exception. Printing stacktrace...");
    stackWalker.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char** argv)
{
    spdlog::info("Setting exception handler...");
    __try
    {
        return Run(argc, argv);
    }
    __except (ExpFilter(GetExceptionInformation(), GetExceptionCode()))
    {
        spdlog::debug("Finished processing SEH exception.");
        return 1;
    }
}
#else
int main(int argc, char** argv)
{
    return Run(argc, argv);
}
#endif
