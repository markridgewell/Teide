
#pragma once

#include <fmt/core.h>

#include <source_location>
#include <string_view>

#ifndef _WIN32
#    include <signal.h>
#endif


namespace Teide
{

#ifdef _WIN32
#    define TEIDE_WHILE_0 __pragma(warning(push)) __pragma(warning(disable : 4127)) while (0) __pragma(warning(pop))
#else
#    define TEIDE_WHILE_0 while (0)
#endif

#if !defined(NDEBUG) && !defined(TEIDE_ASSERTS_ENABLED)
#    define TEIDE_ASSERTS_ENABLED
#endif

using AssertHandler = bool(std::string_view msg, std::string_view expression, std::source_location location);

void SetAssertHandler(AssertHandler* handler = nullptr);

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
        do                                                                                                             \
        {                                                                                                              \
            if (::Teide::Internal::g_handleAssertFail(                                                                 \
                    ::Teide::Internal::AssertFormat(__VA_ARGS__), {}, std::source_location::current()))                \
                TEIDE_BREAK_IMPL();                                                                                    \
        }                                                                                                              \
        TEIDE_WHILE_0

// Break program execution with message if an expression is false
#    define TEIDE_ASSERT(expr, ...)                                                                                    \
        do                                                                                                             \
        {                                                                                                              \
            if (!(expr)                                                                                                \
                && ::Teide::Internal::g_handleAssertFail(                                                              \
                    ::Teide::Internal::AssertFormat(__VA_ARGS__), #expr, std::source_location::current()))             \
                TEIDE_BREAK_IMPL();                                                                                    \
        }                                                                                                              \
        TEIDE_WHILE_0

#else

#    ifdef _WIN32
#        define TEIDE_NOP(x)                                                                                           \
            do                                                                                                         \
            {                                                                                                          \
                __pragma(warning(push));                                                                               \
                __pragma(warning(disable : 6334 4834));                                                                \
                (void)sizeof(x);                                                                                       \
                __pragma(warning(pop));                                                                                \
            }                                                                                                          \
            TEIDE_WHILE_0
#    else
#        define TEIDE_NOP(...)                                                                                         \
            do                                                                                                         \
            {                                                                                                          \
                (void)sizeof(__VA_ARGS__);                                                                             \
            }                                                                                                          \
            TEIDE_WHILE_0
#    endif

#    define TEIDE_BREAK(...) TEIDE_NOP(__VA_ARGS__)
#    define TEIDE_ASSERT(...) TEIDE_NOP(__VA_ARGS__)
#endif

} // namespace Teide
