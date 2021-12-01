
#pragma once

#include <cmath>
#include <concepts>

namespace Geo
{
template <class T>
	requires std::floating_point<T>
constexpr bool Compare(T a, T b, T maxRelDiff) noexcept
{
	return std::abs(a - b) <= maxRelDiff;
}

template <class T>
constexpr T Lerp(T a, T b, T t) noexcept
{
	return a + t * (b - a);
}

template <class T>
constexpr T Midpoint(T a, T b) noexcept
{
	return T{0.5} * (a + b);
}

} // namespace Geo
