
#pragma once

#include <iostream>

namespace Teide
{

constexpr auto UnreachableMessage = "Unreachable code reached!";

#ifndef NDEBUG
// Debug definitions

constexpr bool IsDebugBuild = true;

[[noreturn]] inline void Unreachable()
{
    std::cerr << UnreachableMessage << '\n';
    std::exit(1); // NOLINT(concurrency-mt-unsafe)
}

#else
// Release definitions

constexpr bool IsDebugBuild = false;

[[noreturn]] inline void Unreachable()
{
#    ifdef __GNUC__         // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
    __builtin_unreachable();
#    elif defined(_MSC_VER) // MSVC
    __assume(false);
#    endif
}

#endif
} // namespace Teide
