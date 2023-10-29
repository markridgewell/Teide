
#include "Teide/Assert.h"

#include <algorithm>
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

namespace sv = std::views;

namespace Teide
{
namespace
{
    bool GenericAssertHandler(std::string_view msg, std::string_view expression, SourceLocation location)
    {
        // Print a message
        fmt::print("{}", location.file_name());
        if (location.line() > 0)
        {
            fmt::print("({})", location.line());
        }
        fmt::print(": ");
        if (std::strlen(location.function_name()) > 0)
        {
            fmt::print("{}: ", location.function_name());
        }
        if (expression.empty())
        {
            fmt::println("{}", msg);
        }
        else
        {
            fmt::println("Assertion failed: {}: {}", expression, msg);
        }
        return true;
    }

    bool DefaultAssertHandler(std::string_view msg, std::string_view expression, SourceLocation location)
    {
#ifdef _WIN32
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
        if (MessageBox(nullptr, outputString.c_str(), title, MB_ICONSTOP | MB_OKCANCEL) == IDOK)
        {
            return true;
        }
        return false;
#else
        return GenericAssertHandler(msg, expression, location);
#endif
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
} // namespace Teide
