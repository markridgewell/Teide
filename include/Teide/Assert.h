
#pragma once

#include "Teide/BasicTypes.h"

#include <fmt/core.h>

#include <source_location>
#include <string_view>

#ifdef _WIN32
extern "C" {
int IsDebuggerPresent();
}
#else
#    include <csignal>
#endif

namespace Teide
{

#if !defined(NDEBUG) && !defined(TEIDE_ASSERTS_ENABLED)
#    define TEIDE_ASSERTS_ENABLED
#endif

class SourceLocation
{
public:
    SourceLocation(uint32 line, uint32 column, const char* file, const char* function) :
        m_line{line}, m_column{column}, m_file{file}, m_function{function}
    {}

    constexpr uint32 line() const noexcept { return m_line; }
    constexpr uint32 column() const noexcept { return m_column; }
    constexpr const char* file_name() const noexcept { return m_file; }
    constexpr const char* function_name() const noexcept { return m_function; }

private:
    uint32 m_line;
    uint32 m_column;
    const char* m_file;
    const char* m_function;
};

#if defined(__clang__) || defined(__GNUC__)
#    define TEIDE_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#    define TEIDE_PRETTY_FUNCTION __FUNCSIG__
#else
#    define TEIDE_PRETTY_FUNCTION __func__
#endif

#define SOURCE_LOCATION_CURRENT()                                                                                      \
    ::Teide::SourceLocation(__LINE__, 0, __FILE__, static_cast<const char*>(TEIDE_PRETTY_FUNCTION))

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
#        define TEIDE_BREAK_IMPL() (IsDebuggerPresent() ? (__debugbreak(), 1) : (std::abort(), 0))
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

#    define TEIDE_NOP(...) static_cast<void>(sizeof(0, __VA_ARGS__))

#    define TEIDE_BREAK(...) TEIDE_NOP(__VA_ARGS__)
#    define TEIDE_ASSERT(...) TEIDE_NOP(__VA_ARGS__)
#endif

} // namespace Teide
