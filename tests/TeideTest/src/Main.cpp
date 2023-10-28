
#include "Teide/TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
// Must go after Windows.h
#    include <DbgHelp.h>

#    pragma comment(lib, "dbghelp.lib")

#    include <bit>

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptions [[maybe_unused]])
{
    spdlog::debug("ExceptionHandler begin");
    const HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    void* stack[100];
    const USHORT numFrames = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(stack)), stack, nullptr);

    constexpr ULONG MaxSymbolNameLength = 255;
    const auto buffer = std::make_unique<std::byte[]>(sizeof(SYMBOL_INFO) + MaxSymbolNameLength);
    SYMBOL_INFO* symbol = new (buffer.get()) SYMBOL_INFO{
        .SizeOfStruct = sizeof(SYMBOL_INFO),
        .MaxNameLen = MaxSymbolNameLength,
    };

    for (USHORT i = 0; i < numFrames; i++)
    {
        SymFromAddr(process, std::bit_cast<DWORD64>(stack[i]), 0, symbol);
        spdlog::error("{}: {} - {:x}", i, symbol->Name, symbol->Address);
    }

    spdlog::default_logger()->flush();

    SymCleanup(process);

    spdlog::debug("ExceptionHandler end");
    return EXCEPTION_CONTINUE_SEARCH;
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

    spdlog::debug("Invoking crash...");
    spdlog::default_logger()->flush();
    int* ptr = nullptr;
    *ptr = 42;
    spdlog::debug("You shouldn't see this!");

    testing::InitGoogleTest(&argc, argv);

    if (spdlog::get_level() > spdlog::level::debug)
    {
        testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
        // gtest demands an owning raw pointer to be passed in here
        listeners.Append(new LogSuppressor); // NOLINT(cppcoreguidelines-owning-memory)
    }

    return RUN_ALL_TESTS();
}
