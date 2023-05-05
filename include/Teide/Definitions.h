
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
    std::cerr << UnreachableMessage << std::endl;
    std::terminate();
}

#else
// Release definitions

constexpr bool IsDebugBuild = false;

#    ifdef __GNUC__         // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
[[noreturn]] inline __attribute__((always_inline)) void Unreachable()
{
    __builtin_unreachable();
}
#    elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void Unreachable()
{
    __assume(false);
}
#    endif

#endif
} // namespace Teide
