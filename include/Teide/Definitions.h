
#pragma once

#include <cstdlib>
#include <iostream>
#include <utility>

namespace Teide
{

#ifndef NDEBUG
// Debug definitions

constexpr bool IsDebugBuild = true;

class UnreachableError // intentionally not derived from std::exception
{};

[[noreturn]] inline void Unreachable()
{
    throw UnreachableError();
}

#else
// Release definitions

constexpr bool IsDebugBuild = false;

[[noreturn]] inline void Unreachable()
{
    std::unreachable();
}

#endif
} // namespace Teide
