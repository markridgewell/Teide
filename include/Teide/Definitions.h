
#pragma once

#include <iostream>
#include <utility>

namespace Teide
{

constexpr auto UnreachableMessage = "Unreachable code reached!";

#ifndef NDEBUG
// Debug definitions

constexpr bool IsDebugBuild = true;

[[noreturn]] inline void Unreachable()
{
    std::cerr << UnreachableMessage << std::endl;
    std::exit(1); // NOLINT(concurrency-mt-unsafe)
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
