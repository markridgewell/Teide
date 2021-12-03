
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
	requires std::floating_point<T>
constexpr T Lerp(T a, T b, T t) noexcept
{
	return a + t * (b - a);
}

template <class T>
	requires std::floating_point<T>
constexpr T Midpoint(T a, T b) noexcept
{
	return T{0.5} * (a + b);
}

template <class T>
struct Traits
{};

template <class T>
using ScalarType = typename Traits<T>::ScalarType;

template <class T>
constexpr int VectorSize = Traits<T>::VectorSize;

} // namespace Geo
