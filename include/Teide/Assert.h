
#pragma once

#include <fmt/core.h>

#include <source_location>
#include <string_view>

#ifndef _WIN32
#    include <signal.h>
#endif


namespace Teide
{

#if !defined(NDEBUG) && !defined(TEIDE_ASSERTS_ENABLED)
#    define TEIDE_ASSERTS_ENABLED
#endif

class SourceLocation
{
public:
    SourceLocation(std::uint_least32_t line, std::uint_least32_t column, const char* file, const char* function) :
        m_line{line}, m_column{column}, m_file{file}, m_function{function}
    {}

    constexpr std::uint_least32_t line() const noexcept { return m_line; }
    constexpr std::uint_least32_t column() const noexcept { return m_column; }
    constexpr const char* file_name() const noexcept { return m_file; }
    constexpr const char* function_name() const noexcept { return m_function; }

private:
    std::uint_least32_t m_line;
    std::uint_least32_t m_column;
    const char* m_file;
    const char* m_function;
};

#define SOURCE_LOCATION_CURRENT() ::Teide::SourceLocation(__LINE__, 0, __FILE__, __func__)

using AssertHandler = bool(std::string_view msg, std::string_view expression, SourceLocation location);

void SetAssertHandler(AssertHandler* handler = nullptr);
void PushAssertHandler(AssertHandler* handler = nullptr);
void PopAssertHandler();

#if defined(TEIDE_ASSERTS_ENABLED)
namespace Internal
{
    extern AssertHandler* g_handleAssertFail;

    template <class... Args>
    inline std::string AssertFormat(const fmt::format_string<Args...>& format, Args&&... fmtArgs)
    {
        return fmt::format(format, std::forward<Args>(fmtArgs)...);
    }

    inline std::string AssertFormat()
    {
        return {};
    }
} // namespace Internal

// Break program execution
#    ifdef _WIN32
#        define TEIDE_BREAK_IMPL() __debugbreak()
#    else
#        define TEIDE_BREAK_IMPL() raise(SIGTRAP)
#    endif

// Break program execution with message
#    define TEIDE_BREAK(...)                                                                                           \
        static_cast<void>(                                                                                             \
            (::Teide::Internal::g_handleAssertFail(                                                                    \
                ::Teide::Internal::AssertFormat(__VA_ARGS__), {}, SOURCE_LOCATION_CURRENT()))                          \
            && (TEIDE_BREAK_IMPL(), false))

// Break program execution with message if an expression is false
#    define TEIDE_ASSERT(expr, ...)                                                                                    \
        (!(expr)                                                                                                       \
         && ::Teide::Internal::g_handleAssertFail(                                                                     \
             ::Teide::Internal::AssertFormat(__VA_ARGS__), #expr, SOURCE_LOCATION_CURRENT())                           \
         && (TEIDE_BREAK_IMPL(), false))

#else

#    define TEIDE_NOP(...) (void)sizeof(0, __VA_ARGS__)

#    define TEIDE_BREAK(...) TEIDE_NOP(__VA_ARGS__)
#    define TEIDE_ASSERT(...) TEIDE_NOP(__VA_ARGS__)
#endif

} // namespace Teide
