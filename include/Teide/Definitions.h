
#pragma once

#include <cstdlib>

#ifndef NDEBUG
// Debug definitions

namespace Teide
{

constexpr bool IsDebugBuild = true;

class UnreachableError // intentionally not derived from std::exception
{};

[[noreturn]] inline void Unreachable()
{
    throw UnreachableError();
}

} // namespace Teide

#else
// Release definitions

#    include <utility>

namespace Teide
{

constexpr bool IsDebugBuild = false;

[[noreturn]] inline void Unreachable()
{
    std::unreachable();
}

} // namespace Teide

#endif
