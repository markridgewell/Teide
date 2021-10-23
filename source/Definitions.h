
#pragma once

#include <exception>

#if _DEBUG
static constexpr bool IsDebugBuild = true;
#else
static constexpr bool IsDebugBuild = false;
#endif

[[noreturn]] inline void Unreachable()
{
	std::terminate();
}
