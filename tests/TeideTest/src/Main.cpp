
#include "Teide/Assert.h"
#include "Teide/TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <exception>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#else
inline bool IsDebuggerPresent()
{
    return false;
}
#endif

namespace
{
bool AssertDie(std::string_view msg, std::string_view expression, std::source_location location)
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
        std::cout << msg << std::endl;
    }
    else
    {
        std::cout << "Assertion failed: " << expression << ": " << msg << std::endl;
    }
    std::terminate();
}

} // namespace

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

    fmt::print("Debugger: {}\n", IsDebuggerPresent());
    if (!IsDebuggerPresent())
    {
        Teide::SetAssertHandler(&AssertDie);
    }

    testing::FLAGS_gtest_break_on_failure = IsDebuggerPresent();

    return RUN_ALL_TESTS();
}
