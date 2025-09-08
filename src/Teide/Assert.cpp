
#include "Teide/Assert.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <ranges>
#include <stack>
#include <string>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#    pragma comment(lib, "user32.lib")
#endif

namespace Teide
{
namespace
{
    bool DefaultAssertHandler(std::string_view msg, std::string_view expression, SourceLocation location)
    {
#ifdef _WIN32
        if (IsDebuggerAttached())
        {
            // Display a message box and give the opportunity to break in the debugger
            std::string outputString;
            auto out = std::back_inserter(outputString);

            fmt::format_to(out, "{}{}", msg, msg.empty() ? "" : "\n");

            if (!expression.empty())
            {
                fmt::format_to(out, "Expression: {}\n", expression);
            }

            fmt::format_to(out, "File: {}\n", std::filesystem::path(location.file_name()).filename().string());
            fmt::format_to(out, "Line: {}\n", location.line());
            fmt::format_to(out, "Function: {}\n", location.function_name());
            fmt::format_to(out, "\nClick OK to break into the debugger.");

            const auto title = expression.empty() ? "Breakpoint triggered!" : "Assert failed!";
            if (MessageBox(nullptr, outputString.c_str(), title, MB_ICONSTOP | MB_OKCANCEL) != IDOK)
            {
                return false;
            }
        }
#endif
        // Print a message
        fmt::print(stderr, "{}", location.file_name());
        if (location.line() > 0)
        {
            fmt::print(stderr, "({})", location.line());
        }
        fmt::print(stderr, ": ");
        if (std::strlen(location.function_name()) > 0)
        {
            fmt::print(stderr, "{}: ", location.function_name());
        }
        if (expression.empty())
        {
            fmt::println(stderr, "{}", msg);
        }
        else
        {
            fmt::println(stderr, "Assertion failed: {}: {}", expression, msg);
        }
        return true;
    }

    std::stack<AssertHandler*> s_handlerStack;
} // namespace

namespace Internal
{
    AssertHandler* g_handleAssertFail = &DefaultAssertHandler;
}

void SetAssertHandler(AssertHandler* handler)
{
    Internal::g_handleAssertFail = handler ? handler : &DefaultAssertHandler;
}

void PushAssertHandler(AssertHandler* handler)
{
    s_handlerStack.push(Internal::g_handleAssertFail);
    SetAssertHandler(handler);
}

void PopAssertHandler()
{
    SetAssertHandler(s_handlerStack.top());
    s_handlerStack.pop();
}

bool IsDebuggerAttached()
{
#ifdef _WIN32
    if (!std::getenv("CI"))
    {
        return IsDebuggerPresent();
    }
#else
    return false;
#endif
}

} // namespace Teide
